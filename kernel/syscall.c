#include "interrupt.h"
#include "sched.h"
#include "malloc.h"

#include "syscall.h"

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
        int call_num, ret;

        /* Other interrupts are allowed while servicing a system call. */
        asm("sti");
        
        callno = e->eax & 0xff;
        if (callno >= sizeof(syscall_vectors) / sizeof(*syscall_vectors)) {
                e->eax = -ENOSYS;
                goto end_syscall;
        }

        e->eax = syscall_vectors[call_num]();

end_syscall:
        asm("cli");
}
