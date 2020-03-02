// Virtual Memory and X86's Segmentation
#include <arch/i386/inc.h>

pde_t entry_pgdir[NPDENTRIES] __attribute__((__aligned__(PGSIZE)));

// Set up GDT for this CPU
void 
seg_init()
{
    // Map "logical" addresses to virtual addresses using identity map.
    struct cpu *c = &cpus[cpuidx()];
    c->gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, PL_KERN);
    c->gdt[SEG_KDATA] = SEG(STA_W        , 0, 0xffffffff, PL_KERN);
    c->gdt[SEG_DCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, PL_DRIVER);
    c->gdt[SEG_DDATA] = SEG(STA_W        , 0, 0xffffffff, PL_DRIVER);
    c->gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, PL_USER);
    c->gdt[SEG_UDATA] = SEG(STA_W        , 0, 0xffffffff, PL_USER);

    extern void loadgdt(void *, int);// in entry.S
    loadgdt(c->gdt, sizeof(c->gdt) - 1);
    //lgdt(c->gdt, sizeof(c->gdt));
}

// Prepare TSS for this process.
void
tss_init()
{
    struct proc *p = thisproc();
    if (p == &thiscpu()->scheduler) return;
    //assert(p->magic == PROC_MAGIC);
    thiscpu()->gdt[SEG_TSS] = SEGTSS(&thiscpu()->ts, sizeof(thiscpu()->ts) - 1, 0);
    thiscpu()->ts.esp0 = (uint32_t)p;
    thiscpu()->ts.ss0 = SEG_SELECTOR(SEG_KDATA, TI_GDT, RPL_KERN);

    // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
    // forbids I/O instructions (e.g., inb and outb) from user space
    thiscpu()->ts.iomb = (uint16_t) 0xFFFF;

    ltr(SEG_TSS << 3);
    //lcr3(V2P(p->pgdir));
}

// Given 'pgdir', a pointer to a page directory, pgdir_walk returns
// a pointer to the page table entry (PTE) for linear address 'va'.
// This requires walking the two-level page table structure.
//
// The relevant page table page might not exist yet.
// If this is true, and alloc == false, then pgdir_walk returns NULL.
// Otherwise, pgdir_walk allocates a new page table page with kalloc.
// 		- If the allocation fails, pgdir_walk returns NULL.
// 		- Otherwise, the new page is cleared, and pgdir_walk returns
//        a pointer into the new page table page.
pte_t *
pgdir_walk(pde_t *pgdir, const void *va, int32_t alloc)
{
    assert((int)pgdir);
    pde_t *pde_p = &pgdir[PDX(va)];
    pte_t *pgt;
    if (*pde_p & PTE_P) 
        pgt = (pte_t *)P2V(PTE_ADDR(*pde_p));
    else if (!alloc || (pgt = (pte_t *)kalloc(PGSIZE)) == 0)
        return 0;
    else {
        memset(pgt, 0, PGSIZE);
        *pde_p = V2P(pgt) | PTE_P | PTE_W | PTE_U;
    }
    return &pgt[PTX(va)];
}

// Allocate a page table for kernel part.
struct vm *
vm_init()
{
    pde_t *pgdir = kalloc(PGSIZE);
    memmove(pgdir, entry_pgdir, sizeof(entry_pgdir));
    return (struct vm *)pgdir;
}

// Switch h/w page table register to the page table.
void
vm_switch(struct vm *pgdir)
{
    lcr3(V2P(pgdir));
}

// Map len bytes beginning at virtual address va 
void
vm_alloc(struct vm *vm, uint32_t va, uint32_t len)
{
    pde_t *pgdir = (void *)vm;
    uint32_t ve;
    va = ROUNDDOWN(va, PGSIZE);
    ve = ROUNDDOWN(va + len - 1, PGSIZE);
    assert(va <= ve);

    while (1) {
        assert(va < KERNBASE);

        pte_t *pte = pgdir_walk(pgdir, (void *)va, 1);
        if (!(*pte & PTE_P)) 
            *pte = V2P(kalloc(PGSIZE)) | PTE_P | PTE_U | PTE_W;
        if (va == ve) 
            break;
        va += PGSIZE;
    }
}

// Unmap len bytes beginning at virtual address va 
// All intersected pages will be freed.
int
vm_dealloc(struct vm *vm, uint32_t va, uint32_t len)
{
    pde_t *pgdir = (void *)vm;
    uint32_t ve;
    va = ROUNDDOWN(va, PGSIZE);
    ve = ROUNDDOWN(va + len - 1, PGSIZE);
    assert(va <= ve);

    while (1) {
        assert(va < KERNBASE);
        assert(va != USTKTOP);

        pte_t *pte = pgdir_walk(pgdir, (void *)va, 1);
        if (*pte & PTE_P) {
            kfree(P2V(PTE_ADDR(*pte)));
            *pte = 0;
        }
        if (va == ve) 
            break;
        va += PGSIZE;
    }
}

/*
// Copy and allocate a new page table 
// that remains the same user data.
// Reusing the kernel space.
struct vm *
vm_fork(struct vm *vm)
{
    //pde_t *pgdir = kalloc(PGSIZE);
    pde_t *pgdir = (void *)vm_init();
    pde_t *pde = pgdir, *opde = opgdir;
    for (; opde < opgdir + PDX(KERNBASE); opde ++, pde ++) {
        if (*opde & PTE_P) {
            pte_t *pgt = kalloc(PGSIZE), *opgt = P2V(PTE_ADDR(*opde));
            for (int i = 0; i < NPDENTRIES; i ++) {
                if (opgt[i] & PTE_P) {
                    void *p = kalloc(PGSIZE);
                    memmove(p, P2V(PTE_ADDR(opgt[i])), PGSIZE);
                    pgt[i] = V2P(p) | PTE_FLAGS(opgt[i]);
                }
                else pgt[i] = 0;
            }
            *pde = V2P(pgt) | PTE_FLAGS(*opde);
        }
        else
            *pde = 0;
    }
    //memmove(pde, opde, (uint32_t)&opgdir[NPDENTRIES] - (uint32_t)opde);
    return (void *)pgdir;
}
*/

// Free the user space of a page table
void 
vm_free(struct vm *vm)
{
    pde_t *pgdir = (void *)vm;
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

// Check that the user has permission to read memory [s, s+len).
// Return 0 if valid else 1.
int 
uvm_check(struct vm *vm, char *s, uint32_t len)
{
    pde_t *pgdir = (void *)vm;
    uint32_t va = ROUNDDOWN((uint32_t)s, PGSIZE);
    uint32_t ve = ROUNDDOWN((uint32_t)s + len - 1, PGSIZE);
    for (uint32_t p = va; p <= ve; p += PGSIZE) {
        if (p >= KERNBASE || !pgdir_walk(pgdir, (void *)p, 0)) {
            cprintf("user has no permission to 0x%x\n", p);
            return 1;
        }
    }
    return 0;
}

void 
vm_test()
{
    //pde_t *pgdir = vm_fork(entry_pgdir);
    //test_pgdir(pgdir);
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
            //cprintf("page table: [0x%x, 0x%x)\n", pgt, (uint32_t)pgt+PGSIZE);

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
    //cprintf("Page tables: %d(%dKB)\n", num_pgt, num_pgt*4);
    cprintf("User's memory: %dMB\n", num_usrpg*4/1024);
    cprintf("***** Test end\n");
}
