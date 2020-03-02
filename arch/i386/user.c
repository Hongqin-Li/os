// User Level Drivers and Services.
#include <arch/i386/inc.h>

#define UCODE_PASTE3(x, y, z) x ## y ## z

#define LOAD_X(name, driver) \
({  \
    extern uint8_t UCODE_PASTE3(_binary_obj_user_, name, _elf_start)[]; \
    struct proc *p = spawnx((void *)UCODE_PASTE3(_binary_obj_user_, name, _elf_start), driver); \
    p; \
})

#define LOAD_USER(name) ({LOAD_X(name, 0); })
#define LOAD_DRIVER(name) ({LOAD_X(name, 1); })

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
    acquire(&ptable.lock);

    //LOAD_USER(test);
    LOAD_USER(fs);

    utable[USER_KBD] = LOAD_DRIVER(kbd); // Keyboard Driver
    utable[USER_VGA] = LOAD_DRIVER(vga); // VGA Driver

    // Map CGA Memory for VGA driver
    pte_t *pte = pgdir_walk((pde_t *)(utable[USER_VGA]->vm), (void *)0xb8000, 1);
    assert(!(*pte & PTE_P));
    *pte = 0xb8000 | PTE_P | PTE_W | PTE_U;

    release(&ptable.lock);
    //proc_stat();
    cprintf("user init finished.\n");
}

