#ifndef __PPC_SYSTEM_H__
#define __PPC_SYSTEM_H__

#include <xen/lib.h>
#include <public/arch-ppc.h>

#define smp_mb()	asm volatile("sync" : : : "memory")
#define smp_wmb()	asm volatile("lwsync" : : : "memory")

#define local_irq_disable()	asm volatile("nop" ::: "memory") // TODO:
#define local_irq_enable()	asm volatile("nop" ::: "memory") // TODO:

#define local_irq_save(x)	asm volatile("nop" ::: "memory") // TODO:
#define local_irq_restore(x)	asm volatile("nop" ::: "memory") // TODO:

static inline int local_irq_is_enabled(void)
{
	return 0; // TODO:
}
#endif /* __PPC_SYSTEM_H__ */
