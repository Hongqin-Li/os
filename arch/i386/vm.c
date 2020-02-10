// Virtual Memory
#include <arch/i386/inc.h>

// Set up GDT for this CPU
void 
seg_init()
{
    // Map "logical" addresses to virtual addresses using identity map.
    struct cpu *c = &cpus[cpuidx()];
    c->gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, DPL_KERN);
    c->gdt[SEG_KDATA] = SEG(STA_W        , 0, 0xffffffff, DPL_KERN);
    c->gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, DPL_USER);
    c->gdt[SEG_UDATA] = SEG(STA_W        , 0, 0xffffffff, DPL_USER);

    extern void loadgdt(void *, int);// in entry.S
    loadgdt(c->gdt, sizeof(c->gdt) - 1);
    //lgdt(c->gdt, sizeof(c->gdt));
}

// Switch h/w page table register to the page table.
void
vm_switch(pde_t *pgdir)
{
    lcr3(V2P(pgdir));
}

// Map va -> pa with permssion.
// Only user-space va is allowed.
void
vm_map(pde_t *pgdir, uint32_t va, uint32_t pa, int perm)
{
    va = ROUNDDOWN(va, PGSIZE);
    pa = ROUNDDOWN(pa, PGSIZE);

    assert(va < KERNBASE);

    pde_t *pde = &pgdir[PDX(va)];
    if (!(*pde & PTE_P)) {
        pte_t *pgt = kalloc(PGSIZE);
        memset(pgt, 0, PGSIZE);
        *pde = V2P(pgt) | PTE_P | PTE_W | PTE_U;
    }
    pte_t *pgt = P2V(PTE_ADDR(*pde));
    pte_t *pte = &pgt[PTX(va)];
    assert(!(*pte & PTE_P));// Remap is not allowed.
    *pte = pa | PTE_P | PTE_U | perm;
}

// Copy and allocate a new page table 
// that remains the same mapping.
// Reusing the kernel space.
pde_t *
vm_fork(pde_t *opgdir)
{
    pde_t *pgdir = kalloc(PGSIZE);
    pde_t *pde = pgdir, *opde = opgdir;
    for (; opde < opgdir + PDX(KERNBASE); opde ++, pde ++) {
        if (*opde & PTE_P) {
            pte_t *pgt = kalloc(PGSIZE);
            memmove(pgt, P2V(PTE_ADDR(*opde)), PGSIZE);
            *pde = V2P(pgt) | PTE_FLAGS(*opde);
        }
        else
            *pde = 0;
    }
    memmove(pde, opde, (uint32_t)&opgdir[NPDENTRIES] - (uint32_t)opde);
    return pgdir;
}

// Free the user space of a page table
void 
vm_free(pde_t *pgdir)
{
    for (int i = 0; i < PDX(KERNBASE); i ++) {
        if (pgdir[i] & PTE_P) {
            pte_t *pgt = P2V(PTE_ADDR(pgdir[i]));
            for (int i = 0; i < NPDENTRIES; i ++) {
                if (pgt[i] & PTE_P) 
                    kfree(P2V(PTE_ADDR(pgt[i])));
            }
            kfree(pgt);
        }
    }
    kfree(pgdir);
}

void 
vm_test()
{
    pde_t *pgdir = vm_fork(entry_pgdir);
    test_pgdir(pgdir);
    vm_map(pgdir, 0xf00000, 0, 0);
    test_pgdir(pgdir);
}

// Print and test a page table, showing something like 0xf0000000...0xfe000000 -> 0x0...0xe000000
// During printing, we test the page table by accessing the value of 
// each page mapped according to the page table. Failed access indicates error.
void 
test_pgdir(pde_t *pgdir) {

    cprintf("***** Test and print page table at 0x%x(va): va -> pa\n", pgdir);

    uint32_t num_pgt = 0;
    uint32_t num_usrpg = 0;

    int init = 0;
    uint32_t vs = 0, ve = 0, ps = 0, pe = 0;
    uint32_t flag = 0;

    for (int i = 0; i < NPDENTRIES; i ++) 

        if (pgdir[i] & PTE_P) {
            num_pgt ++;

            pte_t *pgt = P2V(PTE_ADDR(pgdir[i]));
            cprintf("page table: [0x%x, 0x%x)\n", pgt, (uint32_t)pgt+PGSIZE);

            for (int j = 0; j < NPTENTRIES; j ++) 

                if (pgt[j] & PTE_P) {
                    num_usrpg ++;

                    uint32_t v = (uint32_t)PGADDR(i, j, 0);
                    uint32_t p = (uint32_t)PTE_ADDR(pgt[j]);
                    uint32_t tflag = PTE_FLAGS(pgt[j]) & 0xf;// only check user-defined flag
                    
                    //char temp = *(char *)v; // Make sure that each virtual address is accessiable
                    //temp += 1;

                    if (!init) {
                        ve = (vs = v) + PGSIZE;
                        pe = (ps = p) + PGSIZE;
                        flag = tflag;
                        init = 1;
                    }
                    else if (v == ve && p == pe && tflag == flag) {
                        ve = v + PGSIZE;
                        pe = p + PGSIZE;
                    }
                    else {
                        cprintf("[0x%x...0x%x) -> [0x%x...0x%x): flag 0x%x\n", vs, ve, ps, pe, flag);
                        ve = (vs = v) + PGSIZE;
                        pe = (ps = p) + PGSIZE;
                        flag = tflag;
                    }
                }
        }
    
    cprintf("[0x%x...0x%x) -> [0x%x...0x%x): flag 0x%x\n", vs, ve, ps, pe, flag);
    cprintf("Page tables: %d(%dKB)\n", num_pgt, num_pgt*4);
    cprintf("User's memory: %dMB\n", num_usrpg*4/1024);
    cprintf("***** Test end\n");
}
