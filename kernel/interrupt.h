#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

struct registers {
        int32_t eax;
        int32_t ebx;
        int32_t ecx;
        int32_t edx;
        int32_t esi;
        int32_t edi;
        int32_t esp;
        int32_t ebp;
        int32_t eflags;
        int32_t eip;
        int32_t cs;
        int32_t ds;
        int32_t ss;
        int32_t es;
        int32_t fs;
        int32_t gs;
};

struct exception {
        struct registers regs;
        uint32_t cr0;
        uint32_t cr2;
        uint32_t cr3;
        uint32_t eno;
        uint32_t err;
};

extern struct exception except;

#endif
