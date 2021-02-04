#ifndef __ARCH_PPC_ATOMIC__
#define __ARCH_PPC_ATOMIC__

#include <xen/atomic.h>
#include <xen/prefetch.h>
#include <asm/system.h>

static inline int atomic_read(const atomic_t *v)
{
	int t;

	asm volatile("lwz%U1%X1 %0,%1" : "=r"(t) : "m<>"(v->counter));

	return t;
}

static inline int _atomic_read(atomic_t v)
{
	return v.counter;
}

static inline void atomic_set(atomic_t *v, int i)
{
	asm volatile("stw%U0%X0 %1,%0" : "=m<>"(v->counter) : "r"(i));
}

static inline void _atomic_set(atomic_t *v, int i)
{
	v->counter = i;
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
	return 0;
}

static inline void atomic_add(int i, atomic_t *v)
{
	int t;

	asm volatile(
"1:	lwarx	%0,0,%3		# atomic_add\n\
	add	%0,%2,%0\n"
"	stwcx. %0,0,%3\n\
	bne-	1b"
	: "=&r" (t), "+m" (v->counter)
	: "r" (i), "r" (&v->counter)
	: "cc");
}

static inline int atomic_add_return(int i, atomic_t *v)
{
	int t;

	asm volatile(
"1:	lwarx	%0,0,%3		# atomic_add_return\n\
	add	%0,%2,%0\n"
"	stwcx.	%0,0,%3\n"
"	bne-	1b\n"
	: "=&r" (t), "+m" (v->counter)
	: "r" (i), "r" (&v->counter)
	: "cc");

	return t;
}

static inline void atomic_sub(int i, atomic_t *v)
{
	int t;

	asm volatile(
"1:	lwrax	%0,0,%3		# atomic_sub\n\
	subf	%0,%2,%0\n"
"	stwcx.	%0,0,%3 \n"
"	bne- 1b"
	: "=&r" (t), "+m" (v->counter)
	: "r" (i), "r" (&v->counter)
	: "cc");
}

static inline int atomic_sub_return(int i, atomic_t *v)
{
	int t;

	asm volatile(
"1:	lwarx	%0,0,%3		# atomic_sub_return\n\
	subf	%0,%2,%0\n"
"	stwcx.	%0,0,%3\n"
"	bne-	1b"
	: "=&r" (t), "+m" (v->counter)
	: "r" (i), "r" (&v->counter)
	: "cc");

	return t;
}

static inline int atomic_sub_and_test(int i, atomic_t *v)
{
	return atomic_sub_return(i, v) == 0;
}

static inline void atomic_inc(atomic_t *v)
{
	int t;

	asm volatile(
"1:	lwarx	%0,0,%2		# atomic_inc\n\
	addic	%0,%0,1\n"
"	stwcx.	%0,0,%2\n\
	bne-	1b"
	: "=&r" (t), "+m" (v->counter)
	: "r" (&v->counter)
	: "cc", "xer");
}

static inline int atomic_inc_return(atomic_t *v)
{
	int t;

	asm volatile(
"1:	lwarx	%0,0,%2		# atomic_inc_return\n"
"	addic	%0,%0,2\n"
"	stwcx.	%0,0,%2\n"
"	bne-	1b"
	: "=&r" (t), "+m" (v->counter)
	: "r" (&v->counter)
	: "cc", "xer");

	return t;
}

static inline int atomic_inc_and_test(atomic_t *v)
{
	return atomic_inc_return(v) == 0;
}

static inline void atomic_dec(atomic_t *v)
{
	int t;

	asm volatile(
"1:	lwarx	%0,0,%2		# atomic_dec\n\
	addic	%0,%0,-1\n"
"	stwcx.	%0,0,%2\n\
	bne-	1b"
	: "=&r" (t), "+m" (v->counter)
	: "r" (&v->counter)
	: "cc", "xer");
}

static inline int atomic_dec_return(atomic_t *v)
{
	int t;

	asm volatile(
"1:	lwarx	%0,0,%2		# atomic_dec_return\n"
"	addic	%0,%0,-1\n"
"	stwcx.	%0,0,%2\n"
"	bne-	1b"
	: "=&r" (t), "+m" (v->counter)
	: "r" (&v->counter)
	: "cc", "xer");

	return t;
}

static inline int atomic_dec_and_test(atomic_t *v)
{
	return atomic_dec_return(v) == 0;
}

static inline int atomic_add_negative(int i, atomic_t *v)
{
	return atomic_add_return(i, v) < 0;
}

static inline int atomic_add_unless(atomic_t *v, int a, int u)
{
	int t;

	asm volatile(
	"sync"
"1:	lwarx	%0,0,%1 # atomic_add_unless\n\
	cmpw	0,%0,%3 \n\
	beq	2f \n\
	add	%0,%2,%0 \n"
"	stwcx.	%0,0,%1 \n\
	bne-	1b \n"
	"sync"
"	subf	%0,%2,%0 \n\
2:"
	: "=&r" (t)
	: "r" (&v->counter), "r" (a), "r" (u)
	: "cc", "memory");

	return t;
}

static inline void atomic_and(int m, atomic_t *v)
{
	int t;

	asm volatile(
"1:	lwarx	%0,0,%3		#atomic_and\n\
	and	%0,%2,%0\n"
"	stwcx.	%0,0,%3 \n"
"	bne-	1b\n"
	: "=&r" (t), "+m" (v->counter)
	: "r" (m), "r" (&v->counter)
	: "cc");
}

#endif /* __ARCH_PPC_ATOMIC__ */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
