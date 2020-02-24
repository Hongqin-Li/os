#include <stdio.h>
#include <arch/i386/x86.h>
#include <sys.h>


#define CRTPORT 0x3d4
static uint16_t *crt = (uint16_t *)0xb8000;  // CGA memory

// Intel 8250 serial port (UART).
#define COM1    0x3f8
static int uart;    // is there a uart?

void vga_init();
void vga_putc(char c);

void
umain(int argc, char **argv) 
{
    struct mailbox *mb = (void *)USTKTOP;
    int kbd_pid = USER_PID(USER_KBD);
    cprintf("vga: hello pid %x\n", USER_PID(USER_VGA));
    vga_init();
    while (1) {
        int sender = sys_recv(kbd_pid, sizeof(mb->content));
        //cprintf("sender: %x\n", sender);
        assert(mb->len == 1);
        for (int i = 0; i < mb->len; i ++) {
            //cprintf("%c", mb->content[i]);
            //vga_putc(mb->content[i]);
        }
    }
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
        while(1);
    outb(COM1+0, c);
}


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
    else if(c == '\b') {
        if(pos > 0) --pos;
    } else
        crt[pos++] = (c&0xff) | 0x0700;  // black on white

    if(pos < 0 || pos > 25*80)
        panic("pos under/overflow");

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

void
vga_putc(char c) {
    //uart_putc(c);
    cgaputc(c);
}

void
vga_init() {
    //cga_init();
    uart_init();
    if (!uart)
        cprintf("uart not exists!\n");
    cprintf("console initialized.\n");
}


