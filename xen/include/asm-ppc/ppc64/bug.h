#ifndef __PPC_PPC64_BUG_H__
#define __PPC_PPC64_BUG_H__

#include <xen/stringify.h>
#include <asm/ppc64/brk.h>

#define BUG_INSTR "brk " __stringify(BRK_BUG_FRAME_IMM)

#endif /* __PPC_PPC64_BUG_H__ */
