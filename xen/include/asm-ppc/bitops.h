#ifndef _PPC_BITOPS_H
#define _PPC_BITOPS_H

/* Based on linux/arch/powerpc/include/asm/bitops.h */

/*
 * Non-atomic bit manipulation.
 *
 * Implemented using atomics to be interrupt safe. Could alternatively
 * implement with local interrupt masking
 */
#define __set_bit(n,p)		set_bit(n,p)
#define __clear_bit(n,p)	clear_bit(n,p)

#define BITOP_BITS_PER_WORD	32
#define BITOP_MASK(nr)		(1UL << ((nr) % BITOP_BITS_PER_WORD))
#define BITOP_WORD(nr)		((nr) / BITOP_BITS_PER_WORD)
#define BITS_PER_BYTE		8

#if defined(CONFIG_PPC_64)
# include <asm/ppc64/bitops.h>
#else
# error "unknown PPC variant"
#endif

#define BIT_WORD(nr)	((nr) / BITS_PER_LONG)

/*
 * Atomic bitops
 *
 * The helpers below *should* only be used on memory shared between
 * trusted threads or we know the memory cannot be accessed by another
 * thread.
 */

void set_bit(int nr, volatile void *p);
void clear_bit(int nr, volatile void *p);
void change_bit(int nr, volatile void *p);
int test_and_set_bit(int nr, volatile void *p);
int test_and_clear_bit(int nr, volatile void *p);
int test_and_change_bit(int nr, volatile void *p);

/**
 * __test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is non-atomic and can be reordered.
 * If two examples of this operation race, one can appear to succeed
 * but actually fail. You must protect multiple accesses with a lock.
 */
static inline int __test_and_set_bit(int nr, volatile void *addr)
{
	unsigned int mask = BITOP_MASK(nr);
	volatile unsigned int *p =
		((volatile unsigned int *)addr) + BITOP_WORD(nr);
	unsigned int old = *p;

	*p = old | mask;
	return (old & mask) != 0;
}

/**
 * __test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Adress to count from
 *
 * This operation is non-atomic and can be reordered.
 * If two examples of this operation race, one can appear to succeed
 * but actually fail. You must protect multiple accesses with a lock.
 */
static inline int __test_and_clear_bit(int nr, volatile void *addr)
{
	unsigned int mask = BITOP_MASK(nr);
	volatile unsigned int *p =
		((volatile unsigned int *)addr) + BITOP_WORD(nr);
	unsigned int old = *p;

	*p = old & ~mask;
	return (old & mask) != 0;
}

/**
 * test_bit - Determine whether a bit is set
 * @nr: bit number to test
 * @addr: Address to start counting from
 */
static inline int test_bit(int nr, const volatile void *addr)
{
	const volatile unsigned int *p = (const volatile unsigned int*)addr;
	return 1UL & (p[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}
/*
 * fls: find last (most-significant) bit set
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static inline int fls(unsigned int x)
{
	int ret;

	if (__builtin_constant_p(x))
		return x ? 32 - __builtin_clz(x) : 0;
	asm("cntlzw %0,%1" : "=r" (ret) : "r" (x));
	return 32 - ret;
}

#ifndef find_next_bit
/**
 * find_next_bit - find the next set bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The bitmap size in bits
 */
extern unsigned long find_next_bit(const unsigned long *addr,
				   unsigned long size, unsigned long offset);
#endif

#ifndef find_next_zero_bit
/**
 * find_next_zero_bit - find the next cleared bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The bitmap size in bits
 */
extern unsigned long find_next_zero_bit(const unsigned long *addr,
					unsigned long size,
					unsigned long offset);
#endif

#ifdef CONFIG_GENERIC_FIND_FIRST_BIT

/**
 * find_first_bit - find the first set bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum size to search
 *
 * Returns the bit number of the first set bit.
 */
extern unsigned long find_first_bit(const unsigned long *addr,
				    unsigned long size);


/**
 * find_first_zero_bit - find the first cleared bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum size to search
 *
 * Returns the bit number of the first cleared bit.
 */
extern unsigned long find_first_zero_bit(const unsigned long *addr,
					 unsigned long size);
#else /* CONFIG_GENERIC_FIND_FIRST_BIT */

#define find_first_bit(addr, size) find_next_bit((addr), (size), 0)
#define find_first_zero_bit(addr, size) find_next_zero_bit((addr), (size), 0)

#endif /* CONFIG_GENERIC_FIND_FIRST_BIT */

/**
 * hweightN - returns the hamming weight of a N-bit word
 * @x: the word to weigh
 *
 * The Hamming Weight of a number is the total number of bits set in it.
 */
// TODO: port assembly from linux kernel/elsewhere
#define hweight64(x) generic_hweight64(x)
#define hweight32(x) generic_hweight32(x)
#define hweight16(x) generic_hweight16(x)
#define hweight8(x) generic_hweight8(x)

#endif /* _PPC_BITOPS_H */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
