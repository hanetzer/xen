/*
 * Copyright (C) 2007 Advanced Micro Devices, Inc.
 * Author: Leo Duran <leo.duran@amd.com>
 * Author: Wei Wang <wei.wang2@amd.com> - adapted to xen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 */

#include <xen/acpi.h>
#include <xen/sched.h>
#include <asm/p2m.h>
#include <asm/amd-iommu.h>
#include <asm/hvm/svm/amd-iommu-proto.h>
#include "../ats.h"
#include <xen/pci.h>

/* Given pfn and page table level, return pde index */
static unsigned int pfn_to_pde_idx(unsigned long pfn, unsigned int level)
{
    unsigned int idx;

    idx = pfn >> (PTE_PER_TABLE_SHIFT * (--level));
    idx &= ~PTE_PER_TABLE_MASK;
    return idx;
}

static unsigned int clear_iommu_pte_present(unsigned long l1_mfn,
                                            unsigned long dfn)
{
    struct amd_iommu_pte *table, *pte;
    unsigned int flush_flags;

    table = map_domain_page(_mfn(l1_mfn));
    pte = &table[pfn_to_pde_idx(dfn, 1)];

    flush_flags = pte->pr ? IOMMU_FLUSHF_modified : 0;
    memset(pte, 0, sizeof(*pte));

    unmap_domain_page(table);

    return flush_flags;
}

static unsigned int set_iommu_pde_present(struct amd_iommu_pte *pte,
                                          unsigned long next_mfn,
                                          unsigned int next_level, bool iw,
                                          bool ir)
{
    unsigned int flush_flags = IOMMU_FLUSHF_added;

    if ( pte->pr &&
         (pte->mfn != next_mfn ||
          pte->iw != iw ||
          pte->ir != ir ||
          pte->next_level != next_level) )
            flush_flags |= IOMMU_FLUSHF_modified;

    /*
     * FC bit should be enabled in PTE, this helps to solve potential
     * issues with ATS devices
     */
    pte->fc = !next_level;

    pte->mfn = next_mfn;
    pte->iw = iw;
    pte->ir = ir;
    pte->next_level = next_level;
    pte->pr = 1;

    return flush_flags;
}

static unsigned int set_iommu_pte_present(unsigned long pt_mfn,
                                          unsigned long dfn,
                                          unsigned long next_mfn,
                                          int pde_level,
                                          bool iw, bool ir)
{
    struct amd_iommu_pte *table, *pde;
    unsigned int flush_flags;

    table = map_domain_page(_mfn(pt_mfn));
    pde = &table[pfn_to_pde_idx(dfn, pde_level)];

    flush_flags = set_iommu_pde_present(pde, next_mfn, 0, iw, ir);
    unmap_domain_page(table);

    return flush_flags;
}

void amd_iommu_set_root_page_table(uint32_t *dte, uint64_t root_ptr,
                                   uint16_t domain_id, uint8_t paging_mode,
                                   uint8_t valid)
{
    uint32_t addr_hi, addr_lo, entry;
    set_field_in_reg_u32(domain_id, 0,
                         IOMMU_DEV_TABLE_DOMAIN_ID_MASK,
                         IOMMU_DEV_TABLE_DOMAIN_ID_SHIFT, &entry);
    dte[2] = entry;

    addr_lo = root_ptr & DMA_32BIT_MASK;
    addr_hi = root_ptr >> 32;

    set_field_in_reg_u32(addr_hi, 0,
                         IOMMU_DEV_TABLE_PAGE_TABLE_PTR_HIGH_MASK,
                         IOMMU_DEV_TABLE_PAGE_TABLE_PTR_HIGH_SHIFT, &entry);
    set_field_in_reg_u32(IOMMU_CONTROL_ENABLED, entry,
                         IOMMU_DEV_TABLE_IO_WRITE_PERMISSION_MASK,
                         IOMMU_DEV_TABLE_IO_WRITE_PERMISSION_SHIFT, &entry);
    set_field_in_reg_u32(IOMMU_CONTROL_ENABLED, entry,
                         IOMMU_DEV_TABLE_IO_READ_PERMISSION_MASK,
                         IOMMU_DEV_TABLE_IO_READ_PERMISSION_SHIFT, &entry);
    dte[1] = entry;

    set_field_in_reg_u32(addr_lo >> PAGE_SHIFT, 0,
                         IOMMU_DEV_TABLE_PAGE_TABLE_PTR_LOW_MASK,
                         IOMMU_DEV_TABLE_PAGE_TABLE_PTR_LOW_SHIFT, &entry);
    set_field_in_reg_u32(paging_mode, entry,
                         IOMMU_DEV_TABLE_PAGING_MODE_MASK,
                         IOMMU_DEV_TABLE_PAGING_MODE_SHIFT, &entry);
    set_field_in_reg_u32(IOMMU_CONTROL_ENABLED, entry,
                         IOMMU_DEV_TABLE_TRANSLATION_VALID_MASK,
                         IOMMU_DEV_TABLE_TRANSLATION_VALID_SHIFT, &entry);
    set_field_in_reg_u32(valid ? IOMMU_CONTROL_ENABLED :
                         IOMMU_CONTROL_DISABLED, entry,
                         IOMMU_DEV_TABLE_VALID_MASK,
                         IOMMU_DEV_TABLE_VALID_SHIFT, &entry);
    dte[0] = entry;
}

void iommu_dte_set_iotlb(uint32_t *dte, uint8_t i)
{
    uint32_t entry;

    entry = dte[3];
    set_field_in_reg_u32(!!i, entry,
                         IOMMU_DEV_TABLE_IOTLB_SUPPORT_MASK,
                         IOMMU_DEV_TABLE_IOTLB_SUPPORT_SHIFT, &entry);
    dte[3] = entry;
}

void __init amd_iommu_set_intremap_table(
    uint32_t *dte, uint64_t intremap_ptr, uint8_t int_valid)
{
    uint32_t addr_hi, addr_lo, entry;

    addr_lo = intremap_ptr & DMA_32BIT_MASK;
    addr_hi = intremap_ptr >> 32;

    entry = dte[5];
    set_field_in_reg_u32(addr_hi, entry,
                         IOMMU_DEV_TABLE_INT_TABLE_PTR_HIGH_MASK,
                         IOMMU_DEV_TABLE_INT_TABLE_PTR_HIGH_SHIFT, &entry);
    /* Fixed and arbitrated interrupts remapepd */
    set_field_in_reg_u32(2, entry,
                         IOMMU_DEV_TABLE_INT_CONTROL_MASK,
                         IOMMU_DEV_TABLE_INT_CONTROL_SHIFT, &entry);
    dte[5] = entry;

    set_field_in_reg_u32(addr_lo >> 6, 0,
                         IOMMU_DEV_TABLE_INT_TABLE_PTR_LOW_MASK,
                         IOMMU_DEV_TABLE_INT_TABLE_PTR_LOW_SHIFT, &entry);
    /* 2048 entries */
    set_field_in_reg_u32(0xB, entry,
                         IOMMU_DEV_TABLE_INT_TABLE_LENGTH_MASK,
                         IOMMU_DEV_TABLE_INT_TABLE_LENGTH_SHIFT, &entry);

    /* unmapped interrupt results io page faults*/
    set_field_in_reg_u32(IOMMU_CONTROL_DISABLED, entry,
                         IOMMU_DEV_TABLE_INT_TABLE_IGN_UNMAPPED_MASK,
                         IOMMU_DEV_TABLE_INT_TABLE_IGN_UNMAPPED_SHIFT, &entry);
    set_field_in_reg_u32(int_valid ? IOMMU_CONTROL_ENABLED :
                         IOMMU_CONTROL_DISABLED, entry,
                         IOMMU_DEV_TABLE_INT_VALID_MASK,
                         IOMMU_DEV_TABLE_INT_VALID_SHIFT, &entry);
    dte[4] = entry;
}

void __init iommu_dte_add_device_entry(uint32_t *dte,
                                       struct ivrs_mappings *ivrs_dev)
{
    uint32_t entry;
    uint8_t sys_mgt, dev_ex, flags;
    uint8_t mask = ~(0x7 << 3);

    dte[7] = dte[6] = dte[4] = dte[2] = dte[1] = dte[0] = 0;

    flags = ivrs_dev->device_flags;
    sys_mgt = MASK_EXTR(flags, ACPI_IVHD_SYSTEM_MGMT);
    dev_ex = ivrs_dev->dte_allow_exclusion;

    flags &= mask;
    set_field_in_reg_u32(flags, 0,
                         IOMMU_DEV_TABLE_IVHD_FLAGS_MASK,
                         IOMMU_DEV_TABLE_IVHD_FLAGS_SHIFT, &entry);
    dte[5] = entry;

    set_field_in_reg_u32(sys_mgt, 0,
                         IOMMU_DEV_TABLE_SYS_MGT_MSG_ENABLE_MASK,
                         IOMMU_DEV_TABLE_SYS_MGT_MSG_ENABLE_SHIFT, &entry);
    set_field_in_reg_u32(dev_ex, entry,
                         IOMMU_DEV_TABLE_ALLOW_EXCLUSION_MASK,
                         IOMMU_DEV_TABLE_ALLOW_EXCLUSION_SHIFT, &entry);
    dte[3] = entry;
}

void iommu_dte_set_guest_cr3(uint32_t *dte, uint16_t dom_id, uint64_t gcr3,
                             int gv, unsigned int glx)
{
    uint32_t entry, gcr3_1, gcr3_2, gcr3_3;

    gcr3_3 = gcr3 >> 31;
    gcr3_2 = (gcr3 >> 15) & 0xFFFF;
    gcr3_1 = (gcr3 >> PAGE_SHIFT) & 0x7;

    /* I bit must be set when gcr3 is enabled */
    entry = dte[3];
    set_field_in_reg_u32(IOMMU_CONTROL_ENABLED, entry,
                         IOMMU_DEV_TABLE_IOTLB_SUPPORT_MASK,
                         IOMMU_DEV_TABLE_IOTLB_SUPPORT_SHIFT, &entry);
    /* update gcr3 */
    set_field_in_reg_u32(gcr3_3, entry,
                         IOMMU_DEV_TABLE_GCR3_3_MASK,
                         IOMMU_DEV_TABLE_GCR3_3_SHIFT, &entry);
    dte[3] = entry;

    set_field_in_reg_u32(dom_id, entry,
                         IOMMU_DEV_TABLE_DOMAIN_ID_MASK,
                         IOMMU_DEV_TABLE_DOMAIN_ID_SHIFT, &entry);
    /* update gcr3 */
    entry = dte[2];
    set_field_in_reg_u32(gcr3_2, entry,
                         IOMMU_DEV_TABLE_GCR3_2_MASK,
                         IOMMU_DEV_TABLE_GCR3_2_SHIFT, &entry);
    dte[2] = entry;

    entry = dte[1];
    /* Enable GV bit */
    set_field_in_reg_u32(!!gv, entry,
                         IOMMU_DEV_TABLE_GV_MASK,
                         IOMMU_DEV_TABLE_GV_SHIFT, &entry);

    /* 1 level guest cr3 table  */
    set_field_in_reg_u32(glx, entry,
                         IOMMU_DEV_TABLE_GLX_MASK,
                         IOMMU_DEV_TABLE_GLX_SHIFT, &entry);
    /* update gcr3 */
    set_field_in_reg_u32(gcr3_1, entry,
                         IOMMU_DEV_TABLE_GCR3_1_MASK,
                         IOMMU_DEV_TABLE_GCR3_1_SHIFT, &entry);
    dte[1] = entry;
}

/* Walk io page tables and build level page tables if necessary
 * {Re, un}mapping super page frames causes re-allocation of io
 * page tables.
 */
static int iommu_pde_from_dfn(struct domain *d, unsigned long dfn,
                              unsigned long pt_mfn[])
{
    struct amd_iommu_pte *pde, *next_table_vaddr;
    unsigned long  next_table_mfn;
    unsigned int level;
    struct page_info *table;
    const struct domain_iommu *hd = dom_iommu(d);

    table = hd->arch.root_table;
    level = hd->arch.paging_mode;

    BUG_ON( table == NULL || level < 1 || level > 6 );

    next_table_mfn = mfn_x(page_to_mfn(table));

    if ( level == 1 )
    {
        pt_mfn[level] = next_table_mfn;
        return 0;
    }

    while ( level > 1 )
    {
        unsigned int next_level = level - 1;
        pt_mfn[level] = next_table_mfn;

        next_table_vaddr = map_domain_page(_mfn(next_table_mfn));
        pde = &next_table_vaddr[pfn_to_pde_idx(dfn, level)];

        /* Here might be a super page frame */
        next_table_mfn = pde->mfn;

        /* Split super page frame into smaller pieces.*/
        if ( pde->pr && !pde->next_level && next_table_mfn )
        {
            int i;
            unsigned long mfn, pfn;
            unsigned int page_sz;

            page_sz = 1 << (PTE_PER_TABLE_SHIFT * (next_level - 1));
            pfn =  dfn & ~((1 << (PTE_PER_TABLE_SHIFT * next_level)) - 1);
            mfn = next_table_mfn;

            /* allocate lower level page table */
            table = alloc_amd_iommu_pgtable();
            if ( table == NULL )
            {
                AMD_IOMMU_DEBUG("Cannot allocate I/O page table\n");
                unmap_domain_page(next_table_vaddr);
                return 1;
            }

            next_table_mfn = mfn_x(page_to_mfn(table));
            set_iommu_pde_present(pde, next_table_mfn, next_level, true,
                                  true);

            for ( i = 0; i < PTE_PER_TABLE_SIZE; i++ )
            {
                set_iommu_pte_present(next_table_mfn, pfn, mfn, next_level,
                                      true, true);
                mfn += page_sz;
                pfn += page_sz;
             }

            amd_iommu_flush_all_pages(d);
        }

        /* Install lower level page table for non-present entries */
        else if ( !pde->pr )
        {
            if ( next_table_mfn == 0 )
            {
                table = alloc_amd_iommu_pgtable();
                if ( table == NULL )
                {
                    AMD_IOMMU_DEBUG("Cannot allocate I/O page table\n");
                    unmap_domain_page(next_table_vaddr);
                    return 1;
                }
                next_table_mfn = mfn_x(page_to_mfn(table));
                set_iommu_pde_present(pde, next_table_mfn, next_level, true,
                                      true);
            }
            else /* should never reach here */
            {
                unmap_domain_page(next_table_vaddr);
                return 1;
            }
        }

        unmap_domain_page(next_table_vaddr);
        level--;
    }

    /* mfn of level 1 page table */
    pt_mfn[level] = next_table_mfn;
    return 0;
}

static int update_paging_mode(struct domain *d, unsigned long dfn)
{
    uint16_t bdf;
    void *device_entry;
    unsigned int req_id, level, offset;
    unsigned long flags;
    struct pci_dev *pdev;
    struct amd_iommu *iommu = NULL;
    struct page_info *new_root = NULL;
    struct page_info *old_root = NULL;
    struct amd_iommu_pte *new_root_vaddr;
    unsigned long old_root_mfn;
    struct domain_iommu *hd = dom_iommu(d);

    if ( dfn == dfn_x(INVALID_DFN) )
        return -EADDRNOTAVAIL;
    ASSERT(!(dfn >> DEFAULT_DOMAIN_ADDRESS_WIDTH));

    level = hd->arch.paging_mode;
    old_root = hd->arch.root_table;
    offset = dfn >> (PTE_PER_TABLE_SHIFT * (level - 1));

    ASSERT(spin_is_locked(&hd->arch.mapping_lock) && is_hvm_domain(d));

    while ( offset >= PTE_PER_TABLE_SIZE )
    {
        /* Allocate and install a new root table.
         * Only upper I/O page table grows, no need to fix next level bits */
        new_root = alloc_amd_iommu_pgtable();
        if ( new_root == NULL )
        {
            AMD_IOMMU_DEBUG("%s Cannot allocate I/O page table\n",
                            __func__);
            return -ENOMEM;
        }

        new_root_vaddr = __map_domain_page(new_root);
        old_root_mfn = mfn_x(page_to_mfn(old_root));
        set_iommu_pde_present(new_root_vaddr, old_root_mfn, level,
                              true, true);
        level++;
        old_root = new_root;
        offset >>= PTE_PER_TABLE_SHIFT;
        unmap_domain_page(new_root_vaddr);
    }

    if ( new_root != NULL )
    {
        hd->arch.paging_mode = level;
        hd->arch.root_table = new_root;

        if ( !pcidevs_locked() )
            AMD_IOMMU_DEBUG("%s Try to access pdev_list "
                            "without aquiring pcidevs_lock.\n", __func__);

        /* Update device table entries using new root table and paging mode */
        for_each_pdev( d, pdev )
        {
            if ( pdev->type == DEV_TYPE_PCI_HOST_BRIDGE )
                continue;

            bdf = PCI_BDF2(pdev->bus, pdev->devfn);
            iommu = find_iommu_for_device(pdev->seg, bdf);
            if ( !iommu )
            {
                AMD_IOMMU_DEBUG("%s Fail to find iommu.\n", __func__);
                return -ENODEV;
            }

            spin_lock_irqsave(&iommu->lock, flags);
            do {
                req_id = get_dma_requestor_id(pdev->seg, bdf);
                device_entry = iommu->dev_table.buffer +
                               (req_id * IOMMU_DEV_TABLE_ENTRY_SIZE);

                /* valid = 0 only works for dom0 passthrough mode */
                amd_iommu_set_root_page_table((uint32_t *)device_entry,
                                              page_to_maddr(hd->arch.root_table),
                                              d->domain_id,
                                              hd->arch.paging_mode, 1);

                amd_iommu_flush_device(iommu, req_id);
                bdf += pdev->phantom_stride;
            } while ( PCI_DEVFN2(bdf) != pdev->devfn &&
                      PCI_SLOT(bdf) == PCI_SLOT(pdev->devfn) );
            spin_unlock_irqrestore(&iommu->lock, flags);
        }

        /* For safety, invalidate all entries */
        amd_iommu_flush_all_pages(d);
    }
    return 0;
}

int amd_iommu_map_page(struct domain *d, dfn_t dfn, mfn_t mfn,
                       unsigned int flags, unsigned int *flush_flags)
{
    struct domain_iommu *hd = dom_iommu(d);
    int rc;
    unsigned long pt_mfn[7];

    if ( iommu_use_hap_pt(d) )
        return 0;

    memset(pt_mfn, 0, sizeof(pt_mfn));

    spin_lock(&hd->arch.mapping_lock);

    rc = amd_iommu_alloc_root(hd);
    if ( rc )
    {
        spin_unlock(&hd->arch.mapping_lock);
        AMD_IOMMU_DEBUG("Root table alloc failed, dfn = %"PRI_dfn"\n",
                        dfn_x(dfn));
        domain_crash(d);
        return rc;
    }

    /* Since HVM domain is initialized with 2 level IO page table,
     * we might need a deeper page table for wider dfn now */
    if ( is_hvm_domain(d) )
    {
        if ( update_paging_mode(d, dfn_x(dfn)) )
        {
            spin_unlock(&hd->arch.mapping_lock);
            AMD_IOMMU_DEBUG("Update page mode failed dfn = %"PRI_dfn"\n",
                            dfn_x(dfn));
            domain_crash(d);
            return -EFAULT;
        }
    }

    if ( iommu_pde_from_dfn(d, dfn_x(dfn), pt_mfn) || (pt_mfn[1] == 0) )
    {
        spin_unlock(&hd->arch.mapping_lock);
        AMD_IOMMU_DEBUG("Invalid IO pagetable entry dfn = %"PRI_dfn"\n",
                        dfn_x(dfn));
        domain_crash(d);
        return -EFAULT;
    }

    /* Install 4k mapping */
    *flush_flags |= set_iommu_pte_present(pt_mfn[1], dfn_x(dfn), mfn_x(mfn),
                                          1, (flags & IOMMUF_writable),
                                          (flags & IOMMUF_readable));

    spin_unlock(&hd->arch.mapping_lock);

    return 0;
}

int amd_iommu_unmap_page(struct domain *d, dfn_t dfn,
                         unsigned int *flush_flags)
{
    unsigned long pt_mfn[7];
    struct domain_iommu *hd = dom_iommu(d);

    if ( iommu_use_hap_pt(d) )
        return 0;

    memset(pt_mfn, 0, sizeof(pt_mfn));

    spin_lock(&hd->arch.mapping_lock);

    if ( !hd->arch.root_table )
    {
        spin_unlock(&hd->arch.mapping_lock);
        return 0;
    }

    /* Since HVM domain is initialized with 2 level IO page table,
     * we might need a deeper page table for lager dfn now */
    if ( is_hvm_domain(d) )
    {
        int rc = update_paging_mode(d, dfn_x(dfn));

        if ( rc )
        {
            spin_unlock(&hd->arch.mapping_lock);
            AMD_IOMMU_DEBUG("Update page mode failed dfn = %"PRI_dfn"\n",
                            dfn_x(dfn));
            if ( rc != -EADDRNOTAVAIL )
                domain_crash(d);
            return rc;
        }
    }

    if ( iommu_pde_from_dfn(d, dfn_x(dfn), pt_mfn) || (pt_mfn[1] == 0) )
    {
        spin_unlock(&hd->arch.mapping_lock);
        AMD_IOMMU_DEBUG("Invalid IO pagetable entry dfn = %"PRI_dfn"\n",
                        dfn_x(dfn));
        domain_crash(d);
        return -EFAULT;
    }

    /* mark PTE as 'page not present' */
    *flush_flags |= clear_iommu_pte_present(pt_mfn[1], dfn_x(dfn));

    spin_unlock(&hd->arch.mapping_lock);

    return 0;
}

static unsigned long flush_count(unsigned long dfn, unsigned int page_count,
                                 unsigned int order)
{
    unsigned long start = dfn >> order;
    unsigned long end = ((dfn + page_count - 1) >> order) + 1;

    ASSERT(end > start);
    return end - start;
}

int amd_iommu_flush_iotlb_pages(struct domain *d, dfn_t dfn,
                                unsigned int page_count,
                                unsigned int flush_flags)
{
    unsigned long dfn_l = dfn_x(dfn);

    ASSERT(page_count && !dfn_eq(dfn, INVALID_DFN));
    ASSERT(flush_flags);

    /* Unless a PTE was modified, no flush is required */
    if ( !(flush_flags & IOMMU_FLUSHF_modified) )
        return 0;

    /* If the range wraps then just flush everything */
    if ( dfn_l + page_count < dfn_l )
    {
        amd_iommu_flush_all_pages(d);
        return 0;
    }

    /*
     * Flushes are expensive so find the minimal single flush that will
     * cover the page range.
     *
     * NOTE: It is unnecessary to round down the DFN value to align with
     *       the flush order here. This is done by the internals of the
     *       flush code.
     */
    if ( page_count == 1 ) /* order 0 flush count */
        amd_iommu_flush_pages(d, dfn_l, 0);
    else if ( flush_count(dfn_l, page_count, 9) == 1 )
        amd_iommu_flush_pages(d, dfn_l, 9);
    else if ( flush_count(dfn_l, page_count, 18) == 1 )
        amd_iommu_flush_pages(d, dfn_l, 18);
    else
        amd_iommu_flush_all_pages(d);

    return 0;
}

int amd_iommu_flush_iotlb_all(struct domain *d)
{
    amd_iommu_flush_all_pages(d);

    return 0;
}

int amd_iommu_reserve_domain_unity_map(struct domain *domain,
                                       paddr_t phys_addr,
                                       unsigned long size, int iw, int ir)
{
    unsigned long npages, i;
    unsigned long gfn;
    unsigned int flags = !!ir;
    unsigned int flush_flags = 0;
    int rt = 0;

    if ( iw )
        flags |= IOMMUF_writable;

    npages = region_to_pages(phys_addr, size);
    gfn = phys_addr >> PAGE_SHIFT;
    for ( i = 0; i < npages; i++ )
    {
        unsigned long frame = gfn + i;

        rt = amd_iommu_map_page(domain, _dfn(frame), _mfn(frame), flags,
                                &flush_flags);
        if ( rt != 0 )
            break;
    }

    /* Use while-break to avoid compiler warning */
    while ( flush_flags &&
            amd_iommu_flush_iotlb_pages(domain, _dfn(gfn),
                                        npages, flush_flags) )
        break;

    return rt;
}

/* Share p2m table with iommu. */
void amd_iommu_share_p2m(struct domain *d)
{
    struct domain_iommu *hd = dom_iommu(d);
    struct page_info *p2m_table;
    mfn_t pgd_mfn;

    pgd_mfn = pagetable_get_mfn(p2m_get_pagetable(p2m_get_hostp2m(d)));
    p2m_table = mfn_to_page(pgd_mfn);

    if ( hd->arch.root_table != p2m_table )
    {
        free_amd_iommu_pgtable(hd->arch.root_table);
        hd->arch.root_table = p2m_table;

        /* When sharing p2m with iommu, paging mode = 4 */
        hd->arch.paging_mode = 4;
        AMD_IOMMU_DEBUG("Share p2m table with iommu: p2m table = %#lx\n",
                        mfn_x(pgd_mfn));
    }
}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
