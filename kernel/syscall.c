#include "interrupt.h"
#include "sched.h"
#include "malloc.h"

#include "syscall.h"

extern int _syscall(int32_t header, struct syscall_args *args);

static int no_sys()
{
        return -ENOSYS;
}

/*
 * Sends a message to a specified process. If the target process's message queue
 * is full, this call returns an error and yields the current process's time
 * slice.
 * Arguments:
 *      EAX bits 8-23: Target process ID.
 *      EBX: Argument 1.
 *      ECX: Argument 2.
 *      EDX: Argument 3.
 *      ESI: Argument 4.
 *      EDI: Argument 5.
 */
static int do_send()
{
        struct task *target;
        struct message *msg, *qmsg;
        
        target = get_process((except.regs.eax >> 8) & 0xffff);
        if (!target)
                return -EINVAL;
        if (target->num_messages >= MAX_MESSAGES) {
                schedule();
                return -EAGAIN;
        }
        
        msg = kmalloc(sizeof(*msg), 0);
        if (!msg)
                return -ENOMEM;
        
        msg->pid = current->pid;
        msg->args[0] = except.regs.ebx;
        msg->args[1] = except.regs.ecx;
        msg->args[2] = except.regs.edx;
        msg->args[3] = except.regs.esi;
        msg->args[4] = except.regs.edi;
        msg->next = NULL;

        if (!target->mqueue)
                target->mqueue = msg;
        else {
                qmsg = target->mqueue;
                while (qmsg->next)
                        qmsg = qmsg->next;
                qmsg->next = msg;
        }

        target->num_messages++;
        return SUCCESS;
}

/*
 * Returns the next message from the current process's message queue. If the
 * queue is empty, this call returns an error and yields the current process's
 * time slice.
 * Arguments:
 *      None
 * Returns:
 *      EAX bits 0-15: Sender process ID.
 *      EBX: Argument 1.
 *      ECX: Argument 2.
 *      EDX: Argument 3.
 *      ESI: Argument 4.
 *      EDI: Argument 5.
 */
static int do_recv()
{
        struct message *msg;
        int pid;
        
        msg = current->mqueue;
        if (!msg) {
                schedule();
                return -EAGAIN;
        }
        
        pid = msg->pid;
        except.regs.ebx = msg->args[0];
        except.regs.ecx = msg->args[1];
        except.regs.edx = msg->args[2];
        except.regs.esi = msg->args[3];
        except.regs.edi = msg->args[4];

        current->mqueue = msg->next;
        kfree(msg);
        current->num_messages--;
        return pid;
}

/*
 * Puts the current process to sleep for a specified amount of time. This system
 * call always succeeds.
 * Arguments:
 *      EBX: Unsigned sleep time in ms.
 */
static int do_sleep()
{
        current->state = TASK_SLEEP;
        current->timer = (uint32_t) except.regs.ebx;
        schedule();
        return SUCCESS;
}

static int (*syscall_vectors[])() = {
        no_sys,
        do_send,
        do_recv,
        do_sleep,
};

void handle_syscall()
{
        int call_num, ret;
        struct task *caller;
        
        call_num = except.regs.eax & 0xff;
        if (call_num >= sizeof(syscall_vectors) / sizeof(*syscall_vectors)) {
                except.regs.eax = -ENOSYS;
                return;
        }

        caller = current;
        ret = syscall_vectors[call_num]();
        if (caller == current)
                except.regs.eax = ret;
        else
                caller->regs.eax = ret;
}

/* Below are functions for invoking system calls from kernel processes. */

int sys_send(int pid, struct syscall_args *args)
{
        int ret = -EAGAIN;
        while (ret == -EAGAIN)
                ret = _syscall(SYS_SEND | (pid << 8), args);
        return ret;
}

int sys_recv(struct syscall_args *args)
{
        int ret = -EAGAIN;
        while (ret == -EAGAIN)
                ret = _syscall(SYS_RECV, args);
        return ret;
}

int sys_sleep(int millis)
{
        struct syscall_args args = {
                ._1 = millis,
        };
        return _syscall(SYS_SLEEP, &args);
}
