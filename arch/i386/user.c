// User Level Drivers and Services.
#include <inc/elf.h>
#include <inc/sys.h>
#include <arch/i386/inc.h>
#include <traps.h>

#define TF_USER(_tf) \
({  \
    typeof(_tf) tf = (_tf); \
    tf->ds = SEG_SELECTOR(SEG_UDATA, TI_GDT, PL_USER);  \
    tf->cs = SEG_SELECTOR(SEG_UCODE, TI_GDT, PL_USER);  \
    tf->es = tf->ss = tf->fs = tf->gs = tf->ds;         \
    tf->eflags = FL_IF | FL_IOPL(PL_KERN);              \
})

#define TF_DRIVER(_tf) \
({  \
    typeof(_tf) tf = (_tf); \
    tf->ds = SEG_SELECTOR(SEG_DDATA, TI_GDT, PL_DRIVER); \
    tf->cs = SEG_SELECTOR(SEG_DCODE, TI_GDT, PL_DRIVER); \
    tf->es = tf->ss = tf->fs = tf->gs = tf->ds;        \
    tf->eflags = FL_IF | FL_IOPL(PL_DRIVER);   \
})

#define UCODE_PASTE3(x, y, z) x ## y ## z

#define LOAD_X(name, tfloader) \
({  \
    extern uint8_t UCODE_PASTE3(_binary_obj_user_, name, _elf_start)[]; \
    struct proc *p = proc_alloc();   \
    p->pgdir = vm_fork(entry_pgdir);    \
    tfloader(p->tf); \
    test_pgdir(p->pgdir); \
    ucode_load(p, UCODE_PASTE3(_binary_obj_user_, name, _elf_start)); \
    list_push_back(&ptable.ready_list, &p->pos);    \
    p; \
})

#define LOAD_USER(name) ({LOAD_X(name, TF_USER); })
#define LOAD_DRIVER(name) ({LOAD_X(name, TF_DRIVER); })

static void ucode_load(struct proc *p, uint8_t *binary);

struct proc *utable[NUSERS];

void
user_intr(struct proc *p)
{
    sys_send((int)p, 0);
}

// Load drivers and user-space server
void
user_init()
{
    spinlock_acquire(&ptable.lock);

    LOAD_USER(test);
    LOAD_USER(fs);

    utable[USER_KBD] = LOAD_DRIVER(kbd); // Keyboard Driver
    utable[USER_VGA] = LOAD_DRIVER(vga); // VGA Driver

    // Map CGA Memory for VGA driver
    pte_t *pte = pgdir_walk(utable[USER_VGA]->pgdir, (void *)0xb8000, 1);
    assert(!(*pte & PTE_P));
    *pte = 0xb8000 | PTE_P | PTE_W | PTE_U;

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

    //ipc_init(p);

    cprintf("finish ucode loading.\n");
}


