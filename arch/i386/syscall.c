
#include <inc/syscall.h>
#include <inc/sys.h>
#include <arch/i386/inc.h>
// Print a string to the system console.
// The string is exactly 'len' characters long.
static int
sys_cputs(char *s, size_t len)
{
    if (uvm_check(thisproc()->pgdir, s, len)) 
        return 0;
    for (int i = 0; i < len; i ++) 
        consputc(s[i]);
    return 0;
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	//return cons_getc();
    panic("sys_cgetc() no implemented.\n");
    return 0;
}

static int
sys_exit(void)
{
	exit();
    return 0;
}

static int
sys_fork() {
    return fork();
}

static int
sys_sleep()
{
    spinlock_acquire(&ptable.lock);
    sleep();
    spinlock_release(&ptable.lock);
    return 0;
}

static int
sys_yield() {
    spinlock_acquire(&ptable.lock);
    yield(0);
    spinlock_release(&ptable.lock);
    return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
    switch(syscallno) {
        case SYS_cputs: return sys_cputs((char *)a1, a2);
        case SYS_cgetc: return sys_cgetc(); 

        case SYS_exit:  return sys_exit(); 
        case SYS_sleep: return sys_sleep(); 
        case SYS_fork:  return sys_fork();
        case SYS_yield:  return sys_yield();

        case SYS_send:   return sys_send(a1, a2);
        case SYS_recv:   return sys_recv(a1);
        //case SYS_yield: return sys_yield();

        //case SYS_open: return sys_open((char *)a1, a2);
        //case SYS_close: return sys_close(a1);
        //case SYS_read: return sys_read(a1, (void *)a2, a3);
        //case SYS_write: return sys_write(a1, (void *)a2, a3);
        //case SYS_lseek: return sys_lseek(a1, a2, a3);

        //case SYS_msgsnd: return sys_msgsnd(a1, a2, a3); 
        //case SYS_msgrcv: return sys_msgrcv(a1, a2, a3);
        //case SYS_test: return sys_test();
        default: panic("syscall: not implemented.\n");
    }
    return 0;
}

