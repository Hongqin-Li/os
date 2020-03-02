#include <kern/inc.h>

#include <arch/i386/x86.h>
#include <arch/i386/mmu.h>

void 
acquire(struct spinlock *lk) {
    while(lk->locked || __sync_lock_test_and_set(&lk->locked, 1))
        ;
    assert(!(read_eflags()&FL_IF));
}

void 
release(struct spinlock *lk) {
    assert(!(read_eflags()&FL_IF));
    if (!lk->locked)
        panic("release: not locked\n");
    __sync_lock_test_and_set(&lk->locked, 0);
}

