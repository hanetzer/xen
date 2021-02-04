#ifndef __PPC_TYPES_H
#define __PPC_TYPES_H

#ifndef __ASSEMBLY__

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
#if defined(CONFIG_PPC_32)
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#elif defined(CONFIG_PPC_64)
typedef __signed__ long __s64;
typedef unsigned long __u64;
#endif
#endif

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

#if defined(CONFIG_PPC_64)
typedef signed long s64;
typedef unsigned long u64;
#endif

#if defined (CONFIG_PPC_64)
typedef signed long s64;
typedef unsigned long u64;
typedef u64 vaddr_t;
#define PRIvaddr PRIx64
typedef u64 paddr_t;
#define INVALID_PADDR (~0UL)
#define PRIpaddr "016lx"
typedef u64 register_t;
#define PRIregister "016lx"
#endif

#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ size_t;
#else
typedef unsigned long size_t;
#endif
typedef signed long ssize_t;

#endif /* __ASSEMBLY__ */

#endif /* __PPC_TYPES_H */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
