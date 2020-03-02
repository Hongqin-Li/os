# Context switch
#
#   void swtchc(struct context **old, struct context *new);
# 
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

.globl swtchc
swtchc:
	# Save old callee-saved registers
    push %ebp
    push %ebx
    push %esi
    push %edi

    movl 20(%esp), %eax # eax = old
    movl %esp, (%eax)   # *old = esp

	# Switch stacks
    movl 24(%esp), %esp # esp = new

	# Load new callee-saved registers
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp

	ret
