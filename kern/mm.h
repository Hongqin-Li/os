#ifndef KERN_MM_H
#define KERN_MM_H

#include <inc/types.h>
#include <inc/bitmap.h>
#include <inc/list.h>

// Buddy System
struct buddy_system {
    uint32_t max_order;
    uint32_t start, end;// start/end address of the page region
};
struct buddy_system *buddy_init(void *start, void *end);
void *buddy_alloc(struct buddy_system *, size_t);
int buddy_free(struct buddy_system *, void *);

// Freelist
struct freelist {
    void *next;
    void *start, *end;
};
void free_range(struct freelist *f, void *start, void *end);
void *freelist_alloc(struct freelist *f);
void freelist_free(struct freelist *f, void *v);

#endif
