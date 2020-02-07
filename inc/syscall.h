#ifndef INC_SYSCALL_H
#define INC_SYSCALL_H

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_exit,
    SYS_yield, 
    SYS_fork,

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
