#include <kern/inc.h>
#include <inc/sys.h>

struct ptable ptable;

void 
proc_init()
{
    for (int i = 0; i < PROC_BUCKET_SIZE; i ++)
        list_init(&ptable.hlist[i]);
    list_init(&ptable.ready_list);
    list_init(&ptable.zombie_list);
}

// Free all zombie proc.
// Caller should hold ptable.lock.
void
reapall()
{
    while(!list_empty(&ptable.zombie_list)) {
        struct proc *zp = CONTAINER_OF(list_front(&ptable.zombie_list), struct proc, pos);
        list_drop(&zp->pos);
        reap(zp);
    }
}

// Scheduler routine
void
sched()
{
    acquire(&ptable.lock);
    if (!list_empty(&ptable.ready_list)) {
        struct proc *p = CONTAINER_OF(list_front(&ptable.ready_list), struct proc, pos);
        list_drop(&p->pos);
        assert(!list_empty(&p->pos));
        swtch(p);
    }
    else 
        reapall();
    release(&ptable.lock);
}

// Caller should hold ptable.lock
inline void
sleep()
{
    list_init(&thisproc()->pos);
    swtch(thisched());
}

// Wake up process if it is sleeping
// and insert to ready list
// Caller should hold ptable.lock
inline void
wakeup(struct proc *p)
{
    if (PROC_EXISTS(p) && list_empty(&p->pos)) {
        assert(p != thisproc() && p != thisched());
        list_push_front(&ptable.ready_list, &p->pos);
    }
}

// Sleep and wait for process p.
// Direct swtch if possible.
// Caller should hold ptable.lock
void
yield(struct proc *p)
{
    struct proc *tp = thisproc();
    assert(PROC_EXISTS(p));
    list_push_back(&p->wait_list, &tp->pos);
    if (list_empty(&p->pos)) {
        p->pos.next = 0;
        swtch(p);
    }
    else 
        swtch(thisched());
}

// Serve and return the first process in waiting list
// Caller should hold ptable.lock
struct proc *
serve()
{
    struct proc *tp = thisproc();
    while(list_empty(&tp->wait_list)) 
        sleep();
    struct proc *p = CONTAINER_OF(list_front(&tp->wait_list), struct proc, pos);

    assert(!list_empty(&tp->pos));
    assert(PROC_EXISTS(p) && p != thisproc() && p != thisched());

    list_drop(&p->pos);
    list_push_back(&ptable.ready_list, &p->pos);
    return p;
}

void
exit() 
{
    acquire(&ptable.lock);
    struct proc *tp = thisproc();
    assert(PROC_EXISTS(tp));

    // Wakeup waiters
    struct proc *wp;
    LIST_FOREACH_ENTRY(wp, &tp->wait_list, pos) {
        // FIXME
        wakeup(wp);
    }
    cprintf("exit: proc 0x%x exit.\n", tp);

    list_drop(&tp->hlist);
    list_push_back(&ptable.zombie_list, &tp->pos);

    proc_stat();

    swtch(thisched());
    panic("exit: return\n");
}

void *
sbrk(int n)
{
    struct proc *tp = thisproc();
    int sz = tp->size;
    assert(sz >= 0);
    if (n > 0) 
        vm_alloc(tp->vm, USTKTOP + PGSIZE + sz, n);// PGSIZE for mailbox
    else if (n < 0) {
        if (sz + n < 0) 
            n = -sz;
        int s = ROUNDUP(sz + n, PGSIZE);
        assert(s >= 0);
        vm_dealloc(tp->vm, USTKTOP + PGSIZE + s, -n);
    }
    tp->size = sz + n;
    return (void *)USTKTOP+PGSIZE+sz;
}

struct proc *
spawn(struct elfhdr *elf)
{
    acquire(&ptable.lock);
    struct proc *p = spawnx(elf, 0);
    release(&ptable.lock);
    return p;
}

void
proc_stat()
{
    struct proc *p;
    cprintf("ready_list: ");
    LIST_FOREACH_ENTRY(p, &ptable.ready_list, pos) {
        assert(PROC_EXISTS(p));
        cprintf("0x%x", p);
        if (!list_empty(&p->wait_list)) {
            cprintf("(");
            struct proc *wp;
            LIST_FOREACH_ENTRY(wp, &p->wait_list, pos) {
                cprintf("0x%x, ", p);
            }
            cprintf(")");
        }
        cprintf(", ");
    }
    cprintf("\n");
    cprintf("zombie_list: ");
    LIST_FOREACH_ENTRY(p, &ptable.zombie_list, pos) {
        cprintf("0x%x, ", p);
        assert(!PROC_EXISTS(p));
        assert(p->magic == PROC_MAGIC);
        assert(list_empty(&p->wait_list));
    }
    cprintf("\n");
}

