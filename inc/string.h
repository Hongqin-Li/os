
#ifndef INC_STRING_H
#define INC_STRING_H

#include <inc/types.h>

void* 
memset(void *dst, int c, uint32_t n);
int memcmp(const void *v1, const void *v2, uint32_t n);
void* memmove(void *dst, const void *src, uint32_t n);
// memcpy exists to placate GCC.  Use memmove.
void* memcpy(void *dst, const void *src, uint32_t n);

int strncmp(const char *p, const char *q, uint32_t n);
char* strncpy(char *s, const char *t, int n);
// Like strncpy but guaranteed to NUL-terminate.
char* safestrcpy(char *s, const char *t, int n);
int strlen(const char *s);

#endif
