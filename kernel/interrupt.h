#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

/*
 * This struct is pushed onto the stack whenever a processor exception or
 * interrupt occurs. It contains the state of the task that was interrupted,
 * which is restored after the handler is done. Since each task has its own
 * kernel stack, task switching is done just by swapping the current stack, so
 * that the next task's state will be naturally returned to.
 */
struct exception {
        /* Discarded when returning, but good to have for debug dumps */
        int32_t cr0;
        int32_t cr2;
        int32_t cr3;

        /* Pushed by pusha instruction */
        int32_t edi;
        int32_t esi;
        int32_t ebp;
        int32_t _esp; /* kernel esp after pushing everything below */
        int32_t ebx;
        int32_t edx;
        int32_t ecx;
        int32_t eax;

        /* Segment registers */
        int32_t ds;
        int32_t es;
        int32_t fs;
        int32_t gs;

        /* Exception number and error code */
        int32_t eno;
        int32_t err;

        /* Pushed by CPU during interrupt
           (ss and esp are only valid when coming from a user task) */
        int32_t eip;
        int32_t cs;
        int32_t eflags;
        int32_t esp;
        int32_t ss;
};

#define kernel_exception(e) (e.cs == 0x8)
#define user_exception(e) (e.cs != 0x8)

#endif
