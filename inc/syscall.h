#ifndef INC_SYSCALL_H
#define INC_SYSCALL_H

#define T_SYSCALL   64

/* system call numbers */
enum {
    // Debug
	SYS_cputs = 0,
	SYS_cgetc,

    // Process
	SYS_exit,
    SYS_sleep,
    SYS_yield, 
    SYS_fork,
    SYS_sbrk, 

    // IPC
    SYS_send,
    SYS_recv,

    SYS_open,
    SYS_close,
    SYS_read, 
    SYS_write,
    SYS_lseek,

    SYS_msgsnd,
    SYS_msgrcv, 
    SYS_test, 

	NSYSCALLS
};

#endif /* ! INC_SYSCALL_H */
