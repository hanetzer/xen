#ifndef __PPC_DOMAIN_H__
#define __PPC_DOMAIN_H__

#include <xen/cache.h>
#include <xen/timer.h>
#include <asm/page.h>
#include <public/hvm/params.h>

struct hvm_domain {
	uint64_t params[HVM_NR_PARAMS];
};

#ifdef CONFIG_PPC_64
enum domain_type {
	DOMAIN_32BIT,
	DOMAIN_64BIT,
};
#define is_32bit_domain(d) ((d)->arch.type == DOMAIN_32BIT)
#define is_64bit_domain(d) ((d)->arch.type == DOMAIN_64BIT)
#else
#define is_32bit_domain(d) (1)
#define is_64bit_domain(d) (0)
#endif

struct arch_domain {
#ifdef CONFIG_PPC_64
	enum domain_type type;
#endif
};

struct arch_vcpu {
};
#endif /* __PPC_DOMAIN_H__ */
