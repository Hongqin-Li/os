#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <shell/kbd.h>
#include <x86.h>
#include <sys.h>

#define BACKSPACE  0x100

int kbd_getc();

void
umain(int argc, char **argv) 
{
    cprintf("shell hello\n");
    while (1) {
        int cmd = sys_recv(0, 0);
        for (int c; (c = kbd_getc()) != -1; ) 
            if (c)
                cprintf("%c", c);
    }
}

int
kbd_getc()
{
    static uint32_t shift;
    static uint8_t *charcode[4] = {
        normalmap, shiftmap, ctlmap, ctlmap
    };
    uint32_t st, data, c;

    st = inb(KBSTATP);
    if((st & KBS_DIB) == 0)
        return -1;
    if ((st & KBS_MOUSE))
        return -1;
    data = inb(KBDATAP);

    if(data == 0xE0) {
        shift |= E0ESC;
        return 0;
    } 
    else if(data & 0x80) {
        // Key released
        data = (shift & E0ESC ? data : data & 0x7F);
        shift &= ~(shiftcode[data] | E0ESC);
        return 0;
    } 
    else if(shift & E0ESC) {
        // Last character was an E0 escape; or with 0x80
        data |= 0x80;
        shift &= ~E0ESC;
    }

    shift |= shiftcode[data];
    shift ^= togglecode[data];
    c = charcode[shift & (CTL | SHIFT)][data];
    if(shift & CAPSLOCK) {
        if('a' <= c && c <= 'z')
            c += 'A' - 'a';
        else if('A' <= c && c <= 'Z')
            c += 'a' - 'A';
    }

    //consputc(c == '\x7f' ? BACKSPACE: c);
    return c;
}


