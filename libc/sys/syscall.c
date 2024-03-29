#include <stdio.h>
#include <syscall.h>
#include <types.h>
#include <unistd.h>
#include <sys.h>

int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		     : "=a" (ret)
		     : "i" (T_SYSCALL),
		       "a" (num),
		       "d" (a1),
		       "c" (a2),
		       "b" (a3),
		       "D" (a4),
		       "S" (a5)
		     : "cc", "memory");

	if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void exit() { syscall(SYS_exit, 0, 0, 0, 0, 0, 0); }
int fork() { return syscall(SYS_fork, 0, 0, 0, 0, 0, 0); }
int sleep() { return syscall(SYS_sleep, 0, 0, 0, 0, 0, 0); }
int yield() { return syscall(SYS_yield, 0, 0, 0, 0, 0, 0); }
void *sbrk(int n) { return (void *)syscall(SYS_sbrk, 0, n, 0, 0, 0, 0); }

int
sys_send(int pid, int cnt) {
    return syscall(SYS_send, 0, pid, cnt, 0, 0, 0);
}

int
sys_recv(int pid, int cnt) {
    return syscall(SYS_recv, 0, pid, cnt, 0, 0, 0);
}

