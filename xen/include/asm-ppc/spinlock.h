#ifndef __PPC_SPINLOCK_H__
#define __PPC_SPINLOCK_H__

#define arch_lock_acquire_barrier() smp_mb()
#define arch_lock_release_barrier() smp_mb()
#endif /* __PPC_SPINLOCK_H__ */
