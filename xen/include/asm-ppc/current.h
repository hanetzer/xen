#ifndef __PPC_CURRENT_H__
#define __PPC_CURRENT_H__

#include <xen/page-size.h>
#include <xen/percpu.h>

#include <asm/processor.h>

#ifndef __ASSEMBLY__

struct vcpu;

/* Which VCPU is "current" on this PCPU. */
DECLARE_PER_CPU(struct vcpu *, curr_vcpu);

#define current			(this_cpu(curr_vcpu))
#define set_current(vcpu)	do { current = (vcpu); } while (0)
#define get_cpu_current(cpu)	(per_cpu(curr_vcpu, cpu))

DECLARE_PER_CPU(unsigned int, cpu_id);

#define get_processor_id this_cpu(cpu_id)

#endif /* __ASSEMBLY__ */

#endif /* __PPC_CURRENT_H__ */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
