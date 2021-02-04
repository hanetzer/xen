#ifndef _ASM_PP_NOSPEC_H
#define _ASM_PP_NOSPEC_H

static inline bool evaluate_nospec(bool condition)
{
	return condition;
}

static inline void block_speculation(void)
{
}

#endif /* _ASM_PP_NOSPEC_H */
/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
