#ifndef __PPC_MM_H__
#define __PPC_MM_H__

#include <xen/kernel.h>
#include <asm/page.h>
#include <public/xen.h>
#include <xen/pdx.h>

/* Align Xen to a 2 MiB boundary */
#define XEN_PADDR_ALIGN (1 << 21)

struct page_info
{
    /* Each frame can be threaded onto a doubly-linked list. */
    struct page_list_entry list;

    /* Reference count and various PGC_xxx flags and fields. */
    unsigned long count_info;

    /* Context-dependent fields follow... */
    union {
        /* Page is in use: ((count_info & PGC_count_mask) != 0). */
        struct {
            /* Type reference count and various PGT_xxx flags and fields. */
	    unsigned long type_info;
        } inuse;
        /* Page is on a free list: ((count_info & PGC_count_mask) == 0). */
        union {
	    struct {
	        /*
	         * Index of the first *possibly* unscrubbed page in the buddy.
	         * One more bit than maximum possible order to accomodate
	         * INVALID_DIRTY_IDX.
	         */
#define INVALID_DIRTY_IDX ((1UL << (MAX_ORDER + 1)) -1)
                unsigned long first_dirty:MAX_ORDER + 1;

		/* Do TLBs need flushing for safety before next page use? */
		bool need_tlbflush:1;

#define BUDDY_NOT_SCRUBBING	0
#define BUDDY_SCRUBBING		1
#define BUDDY_SCRUB_ABORT	2
		unsigned long scrub_state:2;
	    };

	    unsigned long val;
	    } free;
    } u;

    union {
        /* Page is in use, but not as a shadow. */
        struct {
	    /* Owner of this page (zero if page is anonymous). */
	    struct domain *domain;
	} inuse;

	/* Page is on a free list. */
	struct {
	    /* Order-size of the free chunk this page is the head of. */
	    unsigned int order;
	} free;
    } v;

    union {
	/*
	 * Timestamp from 'TLB clock', used to avoid extra safety flushes.
	 * Only valid for: a) free pages, and b) pages with zero type count
	 */
	u32 tlbflush_timestamp;
    };
    u64 pad;
};

#define PG_shift(idx)   (BITS_PER_LONG - (idx))
#define PG_mask(x, idx) (x ## UL << PG_shift(idx))

#define PGT_none            PG_mask(0, 1) /* no special uses of this page */
#define PGT_writable_page   PG_mask(1, 1) /* has writable mappings?       */
#define PGT_type_mask       PG_mask(1, 1) /* Bits 31 or 64.               */

/* Count of uses of this frame as its current type. */
#define PGT_count_width     PG_shift(2)
#define PGT_count_mask      ((1UL<<PGT_count_width)-1)

/* Clared when the owning guest 'frees' this page. */
#define _PGC_allocated      PG_shift(1)
#define PGC_allocated       PG_mask(1, 1)
/* Page is Xen heap? */
#define _PGC_xen_heap       PG_shift(2)
#define PGC_xen_heap        PG_mask(1, 2)
/* ... */
/* Page is broken? */
#define _PGC_broken         PG_shift(7)
#define PGC_broken          PG_mask(1,7)
/* Mutually-exclusive page states: { inuse, offlining, offlined, free }. */
#define PGC_state           PG_mask(3, 9)
#define PGC_state_inuse     PG_mask(0, 9)
#define PGC_state_offlining PG_mask(1, 9)
#define PGC_state_offlined  PG_mask(2, 9)
#define PGC_state_free      PG_mask(3, 9)
#define page_state_is(pg, st) (((pg)->count_info&PGC_state) == PGC_state##st)
/* Page is not reference counted */
#define _PGC_extra          PG_shift(10)
#define PGC_extra           PG_mask(1, 10)

/* Count of references to this frame. */
#define PGC_count_width     PG_shift(10)
#define PGC_count_mask      ((1UL<<PGC_count_width)-1)

/*
 * Page needs to be scrubbed. Since this bit can only be set on a page that is
 * free (i.e. PGC_state_free) we can reuse PGC_allocated bit.
 */
#define _PGC_need_scrub	_PGC_allocated
#define PGC_need_scrub	PGC_allocated

extern mfn_t xenheap_mfn_start, xenheap_mfn_end;
extern vaddr_t xenheap_virt_end;
#ifdef CONFIG_PPC_64
extern vaddr_t xenheap_virt_start;
extern unsigned long xenheap_base_pdx;
#endif

#ifdef CONFIG_PPC_64
#define is_xen_heap_page(page) ((page)->count_info & PGC_xen_heap)
#define is_xen_heap_mfn(mfn) \
    (mfn_valid(mfn) && is_xen_heap_page(mfn_to_page(mfn)))
#endif

#endif /* __PPC_MM_H__ */
