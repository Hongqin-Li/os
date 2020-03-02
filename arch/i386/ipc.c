#include <arch/i386/inc.h>

void
ipc_init(struct proc *p) {
    vm_alloc(p->vm, USTKTOP, PGSIZE);
    pte_t *pte = pgdir_walk((pde_t *)(p->vm), (void *)USTKTOP, 0);
    assert(*pte & PTE_P);
    p->mailbox = P2V(PTE_ADDR(*pte));
    assert(sizeof(struct mailbox) <= PGSIZE && sizeof(p->mailbox->content) >= sizeof(utable));
    memmove(p->mailbox->content, utable, sizeof(utable));
}

