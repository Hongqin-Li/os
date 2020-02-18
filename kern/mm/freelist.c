#include <kern/mm.h>
#include <kern/console.h>

#define PGSIZE 4096

void
free_range(struct freelist *f, void *start, void *end)
{
	char *p;
    int cnt = 0;
	p = ROUNDUP((char *)start, PGSIZE);
	for (; p + PGSIZE <= (char *)end; p += PGSIZE, cnt ++) 
		freelist_free(f, p);
    cprintf("free_range: 0x%x ~ 0x%x, %d pages\n", start, end, cnt);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
freelist_alloc(struct freelist *f)
{
    void *p = f->next;
    if (p)
        f->next = *(void **)p;
    return p;
}

// Free the page of physical memory pointed at by v.
void
freelist_free(struct freelist *f, void *v)
{
    *(void **)v = f->next;
    f->next = v;
}
