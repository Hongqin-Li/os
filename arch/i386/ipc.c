#include <arch/i386/inc.h>

int 
sendi(struct proc *p, int i)
{
    cprintf("sendi: %x to %x\n", thisproc(), p);
    spinlock_acquire(&ptable.lock);
    struct proc *tp = thisproc();
    tp->msgi = i;
    int err = yield(p);
    spinlock_release(&ptable.lock);

    if (err)
        return -1;
    else 
        return tp->msgi;
}

int
recvi() {
    spinlock_acquire(&ptable.lock);
    struct proc *p = serve();
    int i = p->msgi;
    p->msgi = sizeof(int);
    spinlock_release(&ptable.lock);
    return i;
}


