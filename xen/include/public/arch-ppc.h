#ifndef __XEN_PUBLIC_ARCH_PPC_H
#define __XEN_PUBLIC_ARCH_PPC_H


#define ___DEFINE_XEN_GUEST_HANDLE(name, type) \
	typedef struct { type *p; } __guest_handle_ ## name

#define __DEFINE_XEN_GUEST_HANDLE(name, type)	\
    ___DEFINE_XEN_GUEST_HANDLE(name, type);	\
    ___DEFINE_XEN_GUEST_HANDLE(const_##name, const type)
#define DEFINE_XEN_GUEST_HANDLE(name)	__DEFINE_XEN_GUEST_HANDLE(name, name)
#define __XEN_GUEST_HANDLE(name)	__guest_handle_ ## name
#define XEN_GUEST_HANDLE(name)		__XEN_GUEST_HANDLE(name)
#define XEN_GUEST_HANDLE_PARAM(name)	XEN_GUEST_HANDLE(name)
#define set_xen_guest_handle_raw(hnd, val) do { (hnd).p = val; } while (0)
#define set_xen_guest_handle(hnd, val) set_xen_guest_handle_raw(hnd, val)

typedef uint64_t xen_pfn_t;
#define PRI_xen_pfn PRIx64
#define PRIu_xen_pfn PRIux64

/*
 * Maximum number of virtual CPUs in legacy multi-processor guests.
 * Only one. All other VCPUs must use VCPUOP_register_vcpu_info.
 */
#define XEN_LEGACY_MAX_VCPUS 1

typedef uint64_t xen_ulong_t;
#define PRI_xen_ulong PRIx64

struct arch_vcpu_info {
};
typedef struct arch_vcpu_info arch_vcpu_info_t;

struct arch_shared_info {
};
typedef struct arch_shared_info arch_shared_info_t;

struct vcpu_guest_context {
};
typedef struct vcpu_guest_context vcpu_guest_context_t;
DEFINE_XEN_GUEST_HANDLE(vcpu_guest_context_t);

struct xen_arch_domainconfig {
};

#ifndef __ASSEMBLY__
/* Stub definition of PMU structure */
typedef struct xen_pmu_arch { uint8_t dummy; } xen_pmu_arch_t;
#endif

#endif /* __XEN_PUBLIC_ARCH_PPC_H */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
