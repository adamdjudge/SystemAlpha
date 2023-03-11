#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

struct syscall_args {
        int32_t _1;
        int32_t _2;
        int32_t _3;
        int32_t _4;
        int32_t _5;
};

enum {
        SYS_SEND = 1,
        SYS_RECV,
        SYS_SLEEP,
};

enum {
        SUCCESS,
        EINVAL,
        ENOSYS,
        EPERM,
        ENOMEM,
        EAGAIN,
};

void handle_syscall();

int sys_send();
int sys_recv();
int sys_sleep(int millis);

#endif
