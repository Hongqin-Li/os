// Physical memory allocator, intended to allocate memory for
// user processes, kernel stacks, page table pages, and pipe
// buffers. Allocates 4096-byte pages.

#include <inc/types.h>
#include <inc/string.h>

#include <kern/kalloc.h>
#include <kern/locks.h>
#include <kern/console.h>

#define PGSIZE 4096

// Free page's list element struct.
// We store each free page's run structure in the free page itself.
struct run {
	struct run *next;
};

struct {
    struct spinlock lock;
	struct run *freelist; // Free list of physical pages
} kmem;

// Free the page of physical memory pointed at by v.
void
kfree(char *v)
{
	struct run *r = (struct run *)v;
	r->next = kmem.freelist;
	kmem.freelist = r;
}

void
free_range(void *vstart, void *vend)
{
	char *p;
	p = ROUNDUP((char *)vstart, PGSIZE);
	for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
		kfree(p);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char *
kalloc(void)
{
    struct run *r = kmem.freelist;
    if (r)
        kmem.freelist = r->next;
    return (char *)r;
}
