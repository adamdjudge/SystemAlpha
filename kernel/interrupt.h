#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

struct registers {
        uint32_t eax;
        uint32_t ebx;
        uint32_t ecx;
        uint32_t edx;
        uint32_t esi;
        uint32_t edi;
        uint32_t esp;
        uint32_t ebp;
        uint32_t eflags;
        uint32_t eip;
        uint32_t cs;
        uint32_t ds;
        uint32_t ss;
        uint32_t es;
        uint32_t fs;
        uint32_t gs;
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
