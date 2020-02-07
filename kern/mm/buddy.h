#ifndef KERN_MM_BUDDY_H
#define KERN_MM_BUDDY_H

#include <inc/types.h>
#include <inc/bitmap.h>
#include <inc/list.h>

struct buddy_system {
    uint32_t max_order;
    uint32_t start, end;// start/end address of the page region
};

struct buddy_system *buddy_init(void *start, void *end);
void *buddy_alloc(struct buddy_system *, size_t);
int buddy_free(struct buddy_system *, void *);

#endif
