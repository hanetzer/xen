/*
 * Generate definitions needed by assembly language modules.
 * This code generates raw asm output which is post-processed
 * to extract and format the required data.
 */
#define COMPILE_OFFSETS

#include <xen/types.h>

#define DEFINE(_sym, _val)                                                 \
    asm volatile ("\n.ascii\"==>#define " #_sym " %0 /* " #_val " */<==\"" \
                  : : "i" (_val) )
#define BLANK()                                                            \
    asm volatile ( "\n.ascii\"==><==\"" : : )
#define OFFSET(_sym, _str, _mem)                                           \
    DEFINE(_sym, offsetof(_str, _mem));

void __dummy__(void)
{

}

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
