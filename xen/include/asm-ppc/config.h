/******************************************************************************
 * config.h
 *
 * A Linux-style configuration list.
 */

#ifndef __PPC_CONFIG_H__
#define __PPC_CONFIG_H__

#if defined(CONFIG_PPC_64)
# define LONG_BYTEORDER 3
# define ELFSIZE 64
#else
# define LONG_BYTEORDER 2
# define ELFSIZE 32
#endif

#define BYTES_PER_LONG (1 << LONG_BYTEORDER)
#define BITS_PER_LONG (BYTES_PER_LONG << 3)
#define POINTER_ALIGN BYTES_PER_LONG

#define BITS_PER_LLONG 64

#define CONFIG_PPC_L1_CACHE_SHIFT 7 // TODO:

#ifdef CONFIG_PPC_64
#define MAX_VIRT_CPUS 128u
#endif

/* #define r0  %r0 */
/* #define r1  %r1 */
/* #define r2  %r2 */
/* #define r3  %r3 */
/* #define r4  %r4 */
/* #define r5  %r5 */
/* #define r6  %r6 */
/* #define r7  %r7 */
/* #define r8  %r8 */
/* #define r9  %r9 */
/* #define r10 %r10 */
/* #define r11 %r11 */
/* #define r12 %r12 */
/* #define r13 %r13 */
/* #define r14 %r14 */
/* #define r15 %r15 */
/* #define r16 %r16 */
/* #define r17 %r17 */
/* #define r18 %r18 */
/* #define r19 %r19 */
/* #define r20 %r20 */
/* #define r21 %r21 */
/* #define r22 %r22 */
/* #define r23 %r23 */
/* #define r24 %r24 */
/* #define r25 %r25 */
/* #define r26 %r26 */
/* #define r27 %r27 */
/* #define r28 %r28 */
/* #define r29 %r29 */
/* #define r30 %r30 */
/* #define r31 %r31 */

/* xen_ulong_t is always 64 bits */
#define BITS_PER_XEN_ULONG 64

#define CONFIG_PAGING_LEVELS 3

#define CONFIG_PPC 1

/* #define CONFIG_SMP 1 */

/* Linkage for PPC */
#ifdef __ASSEMBLY__
#define FIXUP_ENDIAN	\
  tdi   0, 0, 0x48; /* Reverse endian of b . + 8         */	\
  b     $+36;       /* Skip trampoline if endian is good */	\
  .long 0x05009f42; /* bcl 20,31,$+4                     */	\
  .long 0xa602487d; /* mflr r10                          */	\
  .long 0x1c004a39; /* addi r10,r10,28 */	\
  .long 0xa600607d; /* mfmsr r11 */	\
  .long 0x01006b69; /* xori r11,r11,1 */	\
  .long 0xa6035a7d; /* mtsrr0 r10*/	\
  .long 0xa6037b7d; /* mtsrr1 r11*/	\
  .long 0x2400004c  /* rfid */
#define ALIGN .align 2
#define ENTRY(name)	\
  .globl name;		\
  ALIGN;		\
  name:
#define GLOBAL(name)	\
  .globl name;		\
  name:
#define END(name)	\
  .size name, .-name
#define ENDPROC(name) \
  .type name, %function; \
  END(name)
#endif /* __ASSEMBLY__ */

#include <xen/const.h>

/* Fixmap slots */
#define FIXMAP_CONSOLE 0 /* The primary UART */
#define FIXMAP_MISC    1 /* Ephemeral mappings of hardware */

#define PAGE_SHIFT 12
#define PAGE_SIZE (_AC(1,L) << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE-1))
#define PAGE_MASK_FLAG (~0)

#define NR_hypercalls 64

#define STACK_ORDER 3
#define STACK_SIZE (PAGE_SIZE << STACK_ORDER)

#ifndef __ASSEMBLY__
extern unsigned long xen_phys_start;
extern unsigned long xenheap_phys_end;
extern unsigned long frametable_virt_end;
#endif

#define watchdog_disable() ((void)0)
#define watchdog_enable()  ((void)0)

#endif /* __PPC_CONFIG_H__ */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
