#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <kernel/types.h>

/* Interrupt numbers */
enum {
	INUM_DIVISION_BY_ZERO,
	INUM_DEBUG,
	INUM_NMI,
	INUM_BREAKPOINT,
	INUM_INTO_DETECTED_OVERFLOW,
	INUM_OUT_OF_BOUNDS,
	INUM_INVALID_OPCODE,
	INUM_NO_COPROCESSOR,
	INUM_DOUBLE_FAULT,
	INUM_COPROCESSOR_SEGMENT_OVERRUN,
	INUM_BAD_TSS,
	INUM_SEGMENT_NOT_PRESENT,
	INUM_STACK_FAULT,
	INUM_GENERAL_PROTECTION_FAULT,
	INUM_PAGE_FAULT,
	INUM_RESERVED,
	INUM_COPROCESSOR_FAULT,
	INUM_ALIGNMENT_CHECK,
	INUM_MACHINE_CHECK,

	INUM_IRQ0 = 32,
	INUM_IRQ1,
	INUM_IRQ2,
	INUM_IRQ3,
	INUM_IRQ4,
	INUM_IRQ5,
	INUM_IRQ6,
	INUM_IRQ7,
	INUM_IRQ8,
	INUM_IRQ9,
	INUM_IRQ10,
	INUM_IRQ11,
	INUM_IRQ12,
	INUM_IRQ13,
	INUM_IRQ14,
	INUM_IRQ15,

	INUM_SYSCALL = 255
};

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

extern void setup_idt();

#endif
