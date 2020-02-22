#ifndef INC_STDLIB_H
#define INC_STDLIB_H

#include <types.h>

void exit();
void *malloc(size_t sz);
void *realloc(void *p, size_t sz);
void free(void *);

#endif
