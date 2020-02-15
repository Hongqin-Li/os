#ifndef INC_STDIO_H
#define INC_STDIO_H

#include <stdarg.h>
#include <stdint.h>

int cprintf(const char *fmt, ...);
void printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list);
int	snprintf(char *str, int size, const char *fmt, ...);
int	vsnprintf(char *str, int size, const char *fmt, va_list);

void panic(const char *fmt, ...);

#endif 
