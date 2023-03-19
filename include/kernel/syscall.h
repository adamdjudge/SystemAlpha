#ifndef SYSCALL_H
#define SYSCALL_H

#include <kernel/types.h>

struct syscall_args {
        int32_t _1;
        int32_t _2;
        int32_t _3;
        int32_t _4;
        int32_t _5;
};

enum {
        SUCCESS,
        EINVAL,
        ENOSYS,
        EPERM,
        ENOMEM,
        EAGAIN,
};

#endif
