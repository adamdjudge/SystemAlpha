#ifndef IDT_H
#define IDT_H

#include "types.h"

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
	INUM_UNKNOWN_INTERRUPT,
	INUM_COPROCESSOR_FAULT,
	INUM_ALIGNMENT_CHECK,
	INUM_MACHINE_CHECK,

	INUM_ISR0 = 32,
	INUM_ISR1,
	INUM_ISR2,
	INUM_ISR3,
	INUM_ISR4,
	INUM_ISR5,
	INUM_ISR6,
	INUM_ISR7,
	INUM_ISR8,
	INUM_ISR9,
	INUM_ISR10,
	INUM_ISR11,
	INUM_ISR12,
	INUM_ISR13,
	INUM_ISR14,
	INUM_ISR15
};

/* Exception structure pushed to the stack during interrupt handling */
struct exception {
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	uint32_t ss;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t inum;
	uint32_t ecode;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t user_esp;
	uint32_t user_ss;
};

void idt_init();
void idt_install_isr(uint8_t irq_num, void (*handler)(struct exception*));

#endif
