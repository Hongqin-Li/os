#ifndef INC_BITMAP_H
#define INC_BITMAP_H

#include <types.h>

#define BITMAP(name, bits) uint32_t name[ROUNDUP(bits, 32)/32]

static inline void 
bitmap_set(uint32_t bm[], int i, int b) {
    if(b) 
        bm[i/32] |= (1<<(i%32));
    else 
        bm[i/32] &= ~(1<<(i%32));
}

static inline int
bitmap_get(uint32_t bm[], int i) {
    return bm[i/32] & (1<<(i%32));
}

#endif
