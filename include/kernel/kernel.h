#ifndef KERNEL_H
#define KERNEL_H

#include <kernel/types.h>
#include <kernel/util.h>

void kprintf(char *fmt, ...);
void kpanic(char *msg);

#endif
