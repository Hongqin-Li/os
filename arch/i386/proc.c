#include <inc/elf.h>
#include <inc/sys.h>
#include <arch/i386/inc.h>
#include <traps.h>
#include <kern/inc.h>

struct context {
	uint32_t edi;
	uint32_t esi;
	uint32_t ebx;
	uint32_t ebp;
	uint32_t eip;
} __attribute__((packed));

struct ptable ptable;

void trapret(); // In trapasm.S
void swtchc(struct context **old, struct context *new);// In swtchc.S

struct proc *
thisproc()
{
    return thiscpu()->proc;
}

struct proc *
thisched()
{
    return &thiscpu()->scheduler;
}

// Init the scheduler process
void
sched_init()
{
    struct proc *tp = &thiscpu()->scheduler;
    tp->vm = (struct vm *)entry_pgdir;
    thiscpu()->proc = tp;
}

void
scheduler()
{
    while(1) {
        cli();
        sched();
        sti();
    }
}

void
reap(struct proc *p)
{
    cprintf("proc_free: %x\n", p);
    vm_free(p->vm);
    kfree((void *)p + sizeof(struct proc) - KSTKSIZE);

    assert(!list_find(&ptable.hlist[PROC_HASH(p)], &p->hlist));
    assert(!list_find(&ptable.ready_list, &p->hlist));
    assert(!list_find(&ptable.zombie_list, &p->hlist));
    assert(list_empty(&p->wait_list));
}

// Switch to process p
// Call should hold ptable.lock
void
swtch(struct proc *p)
{   
    struct proc *tp = thisproc();
    vm_switch(p->vm);
    thiscpu()->proc = p;
    tss_init();
    //cprintf("swtch: cpu %d, %x -> %x\n", cpuidx(), tp, p);
    swtchc(&tp->context, p->context);
}


void
forkret()
{
    cprintf("forkret\n");
    ipc_init(thisproc());
    tss_init();
    release(&ptable.lock);
}

//  Initial Kernel 
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
// Caller should hold ptable.lock
struct proc *
proc_alloc(uint32_t entry, int driver) {
    struct hackframe {
        struct context context;
        void *retaddr;
        struct trapframe tf;
        struct proc p;
    } __attribute__((packed)) *hf;

    hf = (void *)kalloc(KSTKSIZE) + KSTKSIZE - sizeof(*hf);

    struct trapframe *tf = &hf->tf;

    if (driver) {
        tf->ds = SEG_SELECTOR(SEG_DDATA, TI_GDT, PL_DRIVER); 
        tf->cs = SEG_SELECTOR(SEG_DCODE, TI_GDT, PL_DRIVER); 
        tf->eflags = FL_IF | FL_IOPL(PL_DRIVER);   
    }
    else {
        tf->ds = SEG_SELECTOR(SEG_UDATA, TI_GDT, PL_USER);
        tf->cs = SEG_SELECTOR(SEG_UCODE, TI_GDT, PL_USER); 
        tf->eflags = FL_IF | FL_IOPL(PL_KERN);  
    }
    tf->es = tf->ss = tf->fs = tf->gs = tf->ds;        
    tf->esp = USTKTOP;
    tf->err = 0;
    tf->eip = entry; // To be initialized

    hf->retaddr = trapret;
    hf->context.eip = (uint32_t) forkret;

    struct proc *p = &hf->p;
    p->context = &hf->context; // stack pointer
    p->magic = PROC_MAGIC;
    p->size = 0;
    p->vm = vm_init();

    list_init(&p->pos);
    list_push_back(&ptable.hlist[PROC_HASH(p)], &p->hlist);
    list_init(&p->wait_list);

    return p;
}

struct proc *
spawnx(struct elfhdr *elf, int driver)
{
    if(elf->magic != ELF_MAGIC)
        panic("Not an ELF.");

    struct proghdr *ph = (struct proghdr*)((uint8_t*)elf + elf->phoff);
    struct proghdr *eph = ph + elf->phnum;

    struct proc *p = proc_alloc(elf->entry, driver);

    vm_switch(p->vm);
    // Load each program segment (ignores ph flags).
    for(; ph < eph; ph++) {
        if (ph->type != ELF_PROG_LOAD)
            continue;

        assert(ph->va + ph->memsz > ph->va);
        assert(ph->va + ph->memsz <= KERNBASE);

        vm_alloc(p->vm, ph->va, ph->memsz);

        //copy to proc's virtual memory
        memmove((void *)ph->va, (void *)elf + ph->offset, ph->filesz);
        //BSS initialization
        if(ph->memsz > ph->filesz) 
            memset((void *)ph->va + ph->filesz, 0, ph->memsz - ph->filesz);
    }
    vm_switch(thisproc()->vm);

	// One page for initial stack at va USTACKTOP - PGSIZE.
    vm_alloc(p->vm, USTKTOP - PGSIZE, PGSIZE);

    list_push_back(&ptable.ready_list, &p->pos);
    cprintf("spawnx: finish ucode loading.\n");
    return p;
}

int
fork()
{
    panic("fork: not implemented\n");
    acquire(&ptable.lock);
    struct proc *tp = thisproc();
    //struct proc *p = proc_alloc(0);

    /*
    p->vm = vm_fork(tp->vm);
    //*p->tf = *tp->tf;
    p->tf->eax = 0; // fork return 0 in child

    ipc_init(p);

    list_push_back(&ptable.ready_list, &p->pos);
    */
    release(&ptable.lock);
    return 0;
    //return (int)p;
}


