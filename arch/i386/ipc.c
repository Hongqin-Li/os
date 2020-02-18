#include <arch/i386/inc.h>
#include <inc/sys.h>

void
ipc_init(struct proc *p) {
    vm_alloc(p->pgdir, USTKTOP, PGSIZE);
    pte_t *pte = pgdir_walk(p->pgdir, (void *)USTKTOP, 0);
    assert(*pte & PTE_P);
    p->mailbox = P2V(PTE_ADDR(*pte));
    assert(sizeof(struct mailbox) <= PGSIZE);
}

int
sys_send(int pid, int irqno)
{
    struct proc *p = (struct proc *)pid;
    struct mailbox *tm, *m;
    cprintf("sys_send: pid 0x%x, irqno %d\n", pid, irqno);

    spinlock_acquire(&ptable.lock);
    if (PROC_EXISTS(p)) {
        if (irqno) {
            bitmap_set(p->mailbox->irq, irqno, 1);
            wakeup(p);
        }
        else 
            yield(p);
    }
    spinlock_release(&ptable.lock);
    return 0;
}

int
sys_recv(int irqno)
{
    struct proc *tp = thisproc(), *p;
    struct mailbox *tm = tp->mailbox, *m;
    cprintf("sys_recv: tp 0x%x, irqno %d\n", tp, irqno);
    spinlock_acquire(&ptable.lock);
    if (irqno) {
        while (!bitmap_get(tm->irq, irqno))
            sleep();
    }
    else {
        p = serve();
        m = p->mailbox;
        tm->sender = (int)p;
        memmove(tm->content, m->content, sizeof(tm->content));
    }
    spinlock_release(&ptable.lock);
    return 0;
}

