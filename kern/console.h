#ifndef KERN_CONSOLE_H
#define KERN_CONSOLE_H

#define assert(x)  { if (!(x)) panic("%s:%d: assertion failed.\n", __FILE__, __LINE__);  }

#define BACKSPACE (0x100)

void panic(char *fmt, ...);

void cprintf(char *fmt, ...);
void consputc(int c);
void cons_init();

#endif


