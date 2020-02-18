#include <sys.h>

// Entrypoint - this is where the kernel (or our parent environment)
// starts us running when we are initially loaded into a new environment.
.text
.globl _start
_start:
    // See if we were started with arguments on the stack
    cmpl $USTKTOP, %esp
    jne args_exist

    // If not, push dummy argc/argv arguments.
    // This happens when we are loaded by the kernel,
    // because the kernel does not know about passing arguments.
    pushl $0
    pushl $0

args_exist:
    call libmain

spin:
    jmp spin

