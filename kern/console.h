#ifndef KERN_CONSOLE_H
#define KERN_CONSOLE_H

#define assert(x)  { if (!(x)) { cprintf("%s:%d: assertion failed.\n", __FILE__, __LINE__); while(1); }  }
#define panic(s) { cprintf(#s); cprintf("%s:%d: kernel panic.\n", __FILE__, __LINE__); while(1); }

#define BACKSPACE (0x100)

void cprintf(char *fmt, ...);
void consputc(int c);
void cons_init();

#endif


