#ifndef IDT_H
#define IDT_H

#include "types.h"

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
	INUM_MACHINE_CHECK
};

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

#endif
