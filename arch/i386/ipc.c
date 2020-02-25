#include <arch/i386/inc.h>
#include <inc/sys.h>

void
ipc_init(struct proc *p) {
    vm_alloc(p->pgdir, USTKTOP, PGSIZE);
    pte_t *pte = pgdir_walk(p->pgdir, (void *)USTKTOP, 0);
    assert(*pte & PTE_P);
    p->mailbox = P2V(PTE_ADDR(*pte));
    assert(sizeof(struct mailbox) <= PGSIZE  && sizeof(p->mailbox->content) >= sizeof(utable));
    memmove(p->mailbox->content, utable, sizeof(utable));
}

// Send the first cnt bytes of the content of 
// mailbox to process identified by pid.
// If cnt <= 0, send a signal with number -cnt.
// Return #sent bytes if accepted by receiver
// else -1
int
sys_send(int pid, int cnt)
{
    struct proc *p = (struct proc *)pid;
    struct mailbox *tm;
    int sent = 0;
    spinlock_acquire(&ptable.lock);
    if (PROC_EXISTS(p)) {
        if (cnt <= 0) {
            bitmap_set(p->mailbox->irq, -cnt, 1);
            wakeup(p);
        }
        else {
            assert(p != thisproc());
            tm = thisproc()->mailbox;
            tm->len = MIN(cnt, sizeof(tm->content));
            yield(p);
            sent = tm->len;
        }
    }
    spinlock_release(&ptable.lock);
    return sent;
}

// Receiver cnt bytes from process identified by pid.
// If pid is 0, then receive from anyone.
// If cnt <= 0, receive a signal with number -cnt.
// Return 0 if sending a signal else the pid of sender
int
sys_recv(int pid, int cnt)
{
    struct proc *tp = thisproc(), *p = 0;
    struct mailbox *tm = tp->mailbox, *m;
    spinlock_acquire(&ptable.lock);
    if (cnt <= 0) {
        while (!bitmap_get(tm->irq, -cnt))
            sleep();
        bitmap_set(tm->irq, -cnt, 0);
    }
    else {
        while ((int)(p = serve()) != pid && pid) 
            p->mailbox->len = -1;

        m = p->mailbox;
        m->len = MIN(cnt, m->len);
        memmove(tm->content, m->content, tm->len = m->len);
        //cprintf("sys_recv(cpu %d): %s recv from %s, cnt %d\n", cpuidx(), tp->name, p->name, cnt);
    }
    spinlock_release(&ptable.lock);
    return (int)p;
}

