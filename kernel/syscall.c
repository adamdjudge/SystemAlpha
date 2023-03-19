#include <asm/interrupt.h>
#include <kernel/kernel.h>
#include <kernel/syscall.h>

static int no_sys()
{
        return -ENOSYS;
}

static int (*syscall_vectors[])() = {
        no_sys,
};

/*
 * Entry point for system calls to the kernel from user processes.
 */
void handle_syscall(struct exception *e)
{
        int callno, ret;

        /* Other interrupts are allowed while servicing a system call. */
        asm("sti");
        
        callno = e->eax & 0xff;
        if (callno >= sizeof(syscall_vectors) / sizeof(*syscall_vectors)) {
                e->eax = -ENOSYS;
                goto end_syscall;
        }

        e->eax = syscall_vectors[callno]();

end_syscall:
        asm("cli");
}
