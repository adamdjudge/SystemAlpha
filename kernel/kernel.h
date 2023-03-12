#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

void kprintf(char *fmt, ...);
void kpanic(char *msg);

#endif
