#ifndef SPINLOCK_H
#define SPINLOCK_H

typedef struct {
    volatile int locked;
} spinlock_t;

static inline void spinlock_init(spinlock_t *lock)
{
    lock->locked = 0;
}

static inline void spinlock_acquire(spinlock_t *lock)
{
    // GCC builtin for atomic exchange
    while (__sync_lock_test_and_set(&lock->locked, 1))
    {
        // Pause instruction to prevent CPU pipeline stalling
        __asm__ volatile ("pause");
    }
}

static inline void spinlock_release(spinlock_t *lock)
{
    __sync_lock_release(&lock->locked);
}

#endif
