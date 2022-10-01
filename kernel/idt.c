#include "types.h"
#include "console.h"

#include "idt.h"

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void load_idt();

struct idt_entry {
	uint16_t base_low;
	uint16_t segment;
	uint8_t resvd;
	uint8_t flags;
	uint16_t base_high;
} __attribute__((packed));

struct idt_entry idt[256];

struct {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_ptr;

void idt_set_gate(uint8_t inum, uint32_t base)
{
	idt[inum].base_low = base & 0xffff;
	idt[inum].base_high = (base >> 16) & 0xffff;
	idt[inum].segment = 0x08;
	idt[inum].flags = 0x8e;
}

void idt_init()
{
	idt_ptr.limit = sizeof(idt) - 1;
	idt_ptr.base = (uint32_t) &idt;

	idt_set_gate(0, (uint32_t) isr0);
	idt_set_gate(1, (uint32_t) isr1);
	idt_set_gate(2, (uint32_t) isr2);
	idt_set_gate(3, (uint32_t) isr3);
	idt_set_gate(4, (uint32_t) isr4);
	idt_set_gate(5, (uint32_t) isr5);
	idt_set_gate(6, (uint32_t) isr6);
	idt_set_gate(7, (uint32_t) isr7);
	idt_set_gate(8, (uint32_t) isr8);
	idt_set_gate(9, (uint32_t) isr9);
	idt_set_gate(10, (uint32_t) isr10);
	idt_set_gate(11, (uint32_t) isr11);
	idt_set_gate(12, (uint32_t) isr12);
	idt_set_gate(13, (uint32_t) isr13);
	idt_set_gate(14, (uint32_t) isr14);
	idt_set_gate(15, (uint32_t) isr15);
	idt_set_gate(16, (uint32_t) isr16);
	idt_set_gate(17, (uint32_t) isr17);
	idt_set_gate(18, (uint32_t) isr18);

	load_idt();
}

void dump_exception(struct exception *e)
{
	kprintf("*** exception %d  ecode %d  flags 0x%x  eip 0x%x\n",
		e->inum, e->ecode, e->eflags, e->eip);
	kprintf("eax 0x%x  ebx 0x%x  ecx 0x%x  edx 0x%x\n",
		e->eax, e->ebx, e->ecx, e->edx);
	kprintf("esi 0x%x  edi 0x%x  ebp 0x%x  esp 0x%x\n",
		e->esi, e->edi, e->ebp, e->esp);
	kprintf("cs 0x%w  ds 0x%w  ss 0x%w  es 0x%w  fs 0x%w  gs 0x%w\n",
		e->cs, e->ds, e->ss, e->es, e->fs, e->gs);
}

void handle_interrupt(struct exception e)
{
	switch (e.inum) {
	case INUM_DIVISION_BY_ZERO:
		if (e.cs == 8) {
			dump_exception(&e);
			kpanic("kmode divide by zero exception");
		}
	case INUM_BREAKPOINT:
		if (e.cs == 8) {
			dump_exception(&e);
			kpanic("kmode breakpoint");
		}
	case INUM_OUT_OF_BOUNDS:
		if (e.cs == 8) {
			dump_exception(&e);
			kpanic("kmode out of bounds exception");
		}
	case INUM_INVALID_OPCODE:
		if (e.cs == 8) {
			dump_exception(&e);
			kpanic("kmode invalid opcode");
		}
	case INUM_DOUBLE_FAULT:
		if (e.cs == 8) {
			dump_exception(&e);
			kpanic("kmode double fault exception");
		}
	case INUM_STACK_FAULT:
		if (e.cs == 8) {
			dump_exception(&e);
			kpanic("kmode stack fault exception");
		}
	case INUM_GENERAL_PROTECTION_FAULT:
		if (e.cs == 8) {
			dump_exception(&e);
			kpanic("kmode general protection fault exception");
		}
	case INUM_PAGE_FAULT:
		if (e.cs == 8) {
			dump_exception(&e);
			kpanic("kmode page fault exception");
		}
	case INUM_UNKNOWN_INTERRUPT:
		dump_exception(&e);
		kpanic("unknown interrupt exception");
	default:
		dump_exception(&e);
		kpanic("unhandled exception");
	}
}
