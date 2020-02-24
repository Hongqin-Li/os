#include <inc/elf.h>
#include <inc/sys.h>
#include <arch/i386/inc.h>
#include <traps.h>

struct context {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebx;
	uint32_t ebp;
	uint32_t eip;
} __attribute__((packed));

struct ptable ptable;

void trapret(); // In trapasm.S
void swtch(struct context **old, struct context *new);// In swtch.S

struct proc *
thisproc()
{
    return thiscpu()->proc;
}

void 
proc_init()
{
    for (int i = 0; i < PROC_BUCKET_SIZE; i ++)
        list_init(&ptable.hlist[i]);
    list_init(&ptable.ready_list);
    list_init(&ptable.zombie_list);
}

void
forkret()
{
    cprintf("forkret\n");
    ipc_init(thisproc());
    tss_init();
    spinlock_release(&ptable.lock);
}

//  Initial Kerenl 
//  Stack Layout     
//
//  +------------+  top
//  |  proc      |
//  +------------+
//  | trap frame |
//  +------------+
//  | forkret    |
//  +------------+
//  | context    |
//  +------------+
//  |  ...       |
//  +------------+   bottom
struct proc *
proc_alloc()
{
    struct hackframe {
        struct context context;
        void *retaddr;
        struct trapframe tf;
        struct proc p;
    } __attribute__((packed)) *hf;

    hf = (void *)kalloc(KSTKSIZE) + KSTKSIZE - sizeof(*hf);

    struct trapframe *tf = &hf->tf;
    tf->ds = SEG_SELECTOR(SEG_UDATA, TI_GDT, DPL_USER);
    tf->cs = SEG_SELECTOR(SEG_UCODE, TI_GDT, DPL_USER);
    tf->es = tf->ss = tf->fs = tf->gs = tf->ds;
    tf->esp = USTKTOP;
    tf->eflags = FL_IF;
    tf->eip = 0;// will be initialized by ucode_load
    tf->err = 0;

    hf->retaddr = trapret;
    hf->context.eip = (uint32_t) forkret;

    struct proc *p = &hf->p;
    p->context = &hf->context; // stack pointer
    p->tf = tf;
    p->magic = PROC_MAGIC;
    p->size = 0;
    //p->pgdir = vm_fork(entry_pgdir);

    list_init(&p->pos);
    // insert to hash table
    list_push_back(&ptable.hlist[PROC_HASH(p)], &p->hlist);
    list_init(&p->wait_list);

    cprintf("proc_alloc: 0x%x, hash: %d\n", p, PROC_HASH(p));
    return p;
}

static void
proc_free(struct proc *p)
{
    cprintf("proc_free: %x\n", p);
    vm_free(p->pgdir);
    kfree((void *)p + sizeof(struct proc) - KSTKSIZE);

    assert(!list_find(&ptable.hlist[PROC_HASH(p)], &p->hlist));
    assert(!list_find(&ptable.ready_list, &p->hlist));
    assert(!list_find(&ptable.zombie_list, &p->hlist));
    assert(list_empty(&p->wait_list));
}

// Switch to process p
// Call should hold ptable.lock
void
swtchp(struct proc *p)
{   
    struct proc *tp = thisproc();
    vm_switch(p->pgdir);
    thiscpu()->proc = p;
    cprintf("swtchp: cpu %d, %x(%s) -> %x(%s)\n", cpuidx(), tp, tp->name, p, p->name);
    tss_init();
    swtch(&tp->context, p->context);
    thiscpu()->proc = tp;
    vm_switch(tp->pgdir);
}

// Free all zombie proc.
// Caller should hold ptable.lock.
void
reap()
{
    while(!list_empty(&ptable.zombie_list)) {
        struct proc *zp = CONTAINER_OF(list_front(&ptable.zombie_list), struct proc, pos);
        list_drop(&zp->pos);
        proc_free(zp);
    }
}

int
fork()
{
    spinlock_acquire(&ptable.lock);
    struct proc *tp = thisproc();
    struct proc *p = proc_alloc();

    p->pgdir = vm_fork(tp->pgdir);
    *p->tf = *tp->tf;
    p->tf->eax = 0; // fork return 0 in child

    ipc_init(p);

    list_push_back(&ptable.ready_list, &p->pos);
    spinlock_release(&ptable.lock);
    return (int)p;
}

// Caller should hold ptable.lock
inline void
sleep()
{
    list_init(&thisproc()->pos);
    assert(!(read_eflags() & FL_IF));
    swtchp(&thiscpu()->scheduler);
    assert(!(read_eflags() & FL_IF));
}

// Caller should hold ptable.lock
inline void
wakeup(struct proc *p)
{
    if (PROC_EXISTS(p) && list_empty(&p->pos)) {
        assert(p != thisproc() && p != &thiscpu()->scheduler);
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
    list_push_back(&p->wait_list, &tp->pos);
    if (list_empty(&p->pos)) {
        p->pos.next = 0;
        swtchp(p);
    }
    else swtchp(&thiscpu()->scheduler);
    //swtchp(list_empty(&p->pos) ? p: &thiscpu()->scheduler);
}

// Serve and return the first process in waiting list
// Caller should hold ptable.lock
struct proc *
serve()
{
    struct proc *tp = thisproc();
    while(list_empty(&tp->wait_list)) 
        sleep();
    assert(!list_empty(&tp->pos));
    struct proc *p = CONTAINER_OF(list_front(&tp->wait_list), struct proc, pos);
    assert(p != thisproc() && p != &thiscpu()->scheduler);
    list_drop(&p->pos);
    list_push_back(&ptable.ready_list, &p->pos);
    return p;
}

void
exit() 
{
    spinlock_acquire(&ptable.lock);
    struct proc *tp = thisproc();
    assert(PROC_EXISTS(tp));

    // Wakeup waiters
    struct proc *wp;
    LIST_FOREACH_ENTRY(wp, &tp->wait_list, pos) {
        // FIXME
        wakeup(wp);
    }

    cprintf("exit: proc 0x%x exit.\n", tp);
    proc_stat();

    list_drop(&tp->hlist);
    list_push_back(&ptable.zombie_list, &tp->pos);
    swtchp(&thiscpu()->scheduler);

    panic("exit: return\n");
}

void
scheduler() {
    // Init the scheduler process
    struct proc *tp = &thiscpu()->scheduler;
    tp->pgdir = entry_pgdir;
    thiscpu()->proc = tp;
    memmove(tp->name, "sched", 5);
    tp->name[5] = '0' + cpuidx();
    tp->name[6] = '\0';

    while(1) {
        cli();
        spinlock_acquire(&ptable.lock);
        if (!list_empty(&ptable.ready_list)) {
            struct proc *p = CONTAINER_OF(list_front(&ptable.ready_list), struct proc, pos);
            list_drop(&p->pos);
            assert(!list_empty(&p->pos));
            swtchp(p);
        }
        else reap();
        spinlock_release(&ptable.lock);
        sti();
    }
}

void *
sbrk(int n)
{
    struct proc *tp = thisproc();
    int sz = tp->size;
    assert(sz >= 0);
    if (n > 0) 
        vm_alloc(tp->pgdir, USTKTOP + PGSIZE + sz, n);// PGSIZE for mailbox
    else if (n < 0) {
        if (sz + n < 0) 
            n = -sz;
        int s = ROUNDUP(sz + n, PGSIZE);
        assert(s >= 0);
        vm_dealloc(tp->pgdir, USTKTOP + PGSIZE + s, -n);
    }
    //vm_switch(tp->pgdir);
    cprintf("sbrk: proc %x, n %d, size(%d -> %d), heap(%x~%x)\n", tp, n, sz, sz+n, USTKTOP+PGSIZE, USTKTOP+PGSIZE+sz+n);
    tp->size = sz + n;
    //test_pgdir(tp->pgdir);
    return (void *)USTKTOP+PGSIZE+sz;
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

