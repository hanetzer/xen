#ifndef __PPC_SYSREGS__
#define __PPC_SYSREGS__

#define SPRN_TID	0x90 /* Thread ID Register */

static unsigned long mfspr(const unsigned int spr)
{
	unsigned long val;

	asm volatile("mfspr %0,%1" : "=r"(val) : "i"(spr) : "memory");

	return val;
}
#define READ_SYSREG64(name) ({	\
    uint64_t _r;		\
    asm volatile("mfspr %0, "__stringify(name) : "=r" (_r));	\
    _r; })

#define READ_SYSREG(name)	READ_SYSREG64(name)

#endif /* __PPC_SYSREGS__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
