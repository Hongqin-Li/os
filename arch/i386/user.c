// User Level Drivers and Services.
#include <inc/elf.h>
#include <inc/sys.h>
#include <arch/i386/inc.h>
#include <traps.h>

struct proc *kbd_proc;
static void ucode_load(struct proc *p, uint8_t *binary);

void
user_intr(struct proc *p)
{
    sys_send((int)p, 1);
    //spinlock_acquire(&ptable.lock);
    //wakeup(p);
    //spinlock_release(&ptable.lock);
}

// The first process
void
user_init()
{
    extern uint8_t _binary_obj_user_shell_elf_start[];
    extern uint8_t _binary_obj_user_test_elf_start[];

    struct proc *p;
    struct trapframe *tf;

    spinlock_acquire(&ptable.lock);
    // Test
    p = proc_alloc();
    p->pgdir = vm_fork(entry_pgdir);
    tf = p->tf;
    tf->ds = SEG_SELECTOR(SEG_UDATA, TI_GDT, PL_USER);
    tf->cs = SEG_SELECTOR(SEG_UCODE, TI_GDT, PL_USER);
    tf->es = tf->ss = tf->fs = tf->gs = tf->ds;
    //tf->eflags |= FL_IOPL(PL_DRIVER);
    ucode_load(p, _binary_obj_user_test_elf_start);
    list_push_back(&ptable.ready_list, &p->pos);
    test_pgdir(p->pgdir);

    // Keyboard Driver
    /*
    p = proc_alloc();
    p->pgdir = vm_fork(entry_pgdir);
    tf = p->tf;
    tf->ds = SEG_SELECTOR(SEG_DDATA, TI_GDT, PL_DRIVER);
    tf->cs = SEG_SELECTOR(SEG_DCODE, TI_GDT, PL_DRIVER);
    tf->es = tf->ss = tf->fs = tf->gs = tf->ds;
    tf->eflags |= FL_IOPL(PL_DRIVER);
    ucode_load(p, _binary_obj_user_shell_elf_start);
    list_push_back(&ptable.ready_list, &p->pos);
    kbd_proc = p;
    */

    spinlock_release(&ptable.lock);
    //proc_stat();
    cprintf("user init finished.\n");
}

static void
ucode_load(struct proc *p, uint8_t *binary) 
{
    struct elfhdr *elf = (struct elfhdr*)binary;

    if(elf->magic != ELF_MAGIC)
        panic("Not an ELF.");

    struct proghdr *ph = (struct proghdr*)((uint8_t*)elf + elf->phoff);
    struct proghdr *eph = ph + elf->phnum;


    vm_switch(p->pgdir);
    // Load each program segment (ignores ph flags).
    for(; ph < eph; ph++) {
        if (ph->type != ELF_PROG_LOAD)
            continue;

        assert(ph->va + ph->memsz > ph->va);
        assert(ph->va + ph->memsz <= KERNBASE);

        vm_alloc(p->pgdir, ph->va, ph->memsz);

        //copy to proc's virtual memory
        memmove((void *)ph->va, binary + ph->offset, ph->filesz);
        //BSS initialization
        if(ph->memsz > ph->filesz) 
            memset((void *)ph->va + ph->filesz, 0, ph->memsz - ph->filesz);
    }

    vm_switch(entry_pgdir);

    // Prepare the entry point
    p->tf->eip = elf->entry;

	// One page for initial stack at va USTACKTOP - PGSIZE.
    vm_alloc(p->pgdir, USTKTOP - PGSIZE, PGSIZE);

    ipc_init(p);

    cprintf("finish ucode loading.\n");
}


