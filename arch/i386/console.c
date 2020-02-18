// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.


#include <inc/types.h>
#include <inc/string.h>

#include <kern/locks.h>
#include <kern/console.h>

#include <x86.h>
#include <memlayout.h>

static struct spinlock console_lock;

#define CRTPORT 0x3d4
static uint16_t *crt = (uint16_t *)P2V(0xb8000);  // CGA memory

// Intel 8250 serial port (UART).
#define COM1    0x3f8
static int uart;    // is there a uart?

static void 
delay() {
}

void
uart_init(void) {

    // Turn off the FIFO
    outb(COM1+2, 0);

    // 9600 baud, 8 data bits, 1 stop bit, parity off.
    outb(COM1+3, 0x80);    // Unlock divisor
    outb(COM1+0, 115200/9600);
    outb(COM1+1, 0);
    outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
    outb(COM1+4, 0);
    outb(COM1+1, 0x01);    // Enable receive interrupts.

    // If status is 0xFF, no serial port.
    if(inb(COM1+5) == 0xFF)
        return;
    uart = 1;

    // Acknowledge pre-existing interrupt conditions;
    // enable interrupts.
    inb(COM1+2);
    inb(COM1+0);
    //FIXME
    //ioapic_enable(IRQ_COM1, 0);

}

static void
uart_putc(int c)
{
    int i;

    if(!uart)
        return;
    for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
        delay();
    outb(COM1+0, c);
}

/*
static int
uart_getc(void)
{
    if(!uart)
        return -1;
    if(!(inb(COM1+5) & 0x01))
        return -1;
    return inb(COM1+0);
}

void
uart_intr(void)
{
    console_intr(uartgetc);
}
*/



static void
cgaputc(int c)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == BACKSPACE || c == '\b'){
    if(pos > 0) --pos;
  } else
    crt[pos++] = (c&0xff) | 0x0700;  // black on white

  if(pos < 0 || pos > 25*80)
    //panic("pos under/overflow");
    while(1) ;

  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  crt[pos] = ' ' | 0x0700;
}

static void
printint(int xx, int base, int sign)
{
    static char digits[] = "0123456789abcdef";
    char buf[16];
    int i;
    uint x;

    if(sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do{
        buf[i++] = digits[x % base];
    }while((x /= base) != 0);

    if(sign)
        buf[i++] = '-';

    while(--i >= 0)
        consputc(buf[i]);
}

void
consputc(int c) {

    spinlock_acquire(&console_lock);

    if (c == BACKSPACE) {
        uart_putc('\b'); 
        uart_putc(' '); 
        uart_putc('\b');
    } 
    else
        uart_putc(c);

    cgaputc(c);

    spinlock_release(&console_lock);
}

// Print to the console.
void
cprintf(char *fmt, ...)
{
    int i, c, locking;
    uint *argp;
    char *s;

    if (fmt == 0)
        panic("null fmt");

    argp = (uint*)(void*)(&fmt + 1);
    for(i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if(c != '%') {
            consputc(c);
            continue;
        }
        if (!(c = fmt[++i] & 0xff))
            break;

        switch(c) {
        case 'u':
            printint(*argp++, 10, 0);
            break;
        case 'd':
            printint(*argp++, 10, 1);
            break;
        case 'x':
        case 'p':
            printint(*argp++, 16, 0);
            break;
        case 'c':
            consputc(*argp++);
            break;
        case 's':
            if((s = (char*)*argp++) == 0)
                s = "(null)";
            for(; *s; s++)
                consputc(*s);
            break;
        case '%':
            consputc('%');
            break;
        default:
            // Print unknown % sequence to draw attention.
            consputc('%');
            consputc(c);
            break;
        }
    }
}

void
cons_init() {
    //cga_init();
    uart_init();
    if (!uart)
        cprintf("uart not exists!\n");
    cprintf("console initialized.\n");
}


