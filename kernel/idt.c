#include "kernel.h"
#include "io.h"
#include "sched.h"
#include "syscall.h"

#include "interrupt.h"
#include "idt.h"

/* PIC I/O ports */
#define PIC_MASTER_CMD 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD 0xA0
#define PIC_SLAVE_DATA 0xA1

/* External interrupt entry points defined in interrupts.s, all of which push an
   exception structure to the stack and call handle_interrupt */
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
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
extern void isr_sys();

extern void load_idt();

/* Interrupt descriptor table */
static struct {
	uint16_t base_low;
	uint16_t segment;
	uint8_t resvd;
	uint8_t flags;
	uint16_t base_high;
} __attribute__((packed)) idt[256];

/* IDT pointer, used to indirectly load the IDT via lidt instruction */
struct {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_ptr;

/* Set interrupt gate within the IDT using default settings */
static void idt_set_gate(uint8_t inum, uint32_t base)
{
	idt[inum].base_low = base & 0xffff;
	idt[inum].base_high = (base >> 16) & 0xffff;
	idt[inum].segment = 0x08;
	idt[inum].flags = 0x8e;
}

/* Array of IRQ handlers installable by drivers */
static uint32_t irq_handlers[16] = {0};

/* Install an IRQ handler to be called whenever that IRQ number is fired */
void idt_install_isr(uint8_t irq_num, void (*handler)())
{
	if (irq_num > 15)
		return;
	irq_handlers[irq_num] = (uint32_t) handler;
}

/* Initialize and load the IDT, and enable interrupts */
void idt_init()
{
	/* CPU exception handlers */
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

	/* External interrupt request handlers */
	idt_set_gate(32, (uint32_t) irq0);
	idt_set_gate(33, (uint32_t) irq1);
	idt_set_gate(35, (uint32_t) irq3);
	idt_set_gate(34, (uint32_t) irq2);
	idt_set_gate(36, (uint32_t) irq4);
	idt_set_gate(37, (uint32_t) irq5);
	idt_set_gate(38, (uint32_t) irq6);
	idt_set_gate(39, (uint32_t) irq7);
	idt_set_gate(40, (uint32_t) irq8);
	idt_set_gate(41, (uint32_t) irq9);
	idt_set_gate(42, (uint32_t) irq10);
	idt_set_gate(43, (uint32_t) irq11);
	idt_set_gate(44, (uint32_t) irq12);
	idt_set_gate(45, (uint32_t) irq13);
	idt_set_gate(46, (uint32_t) irq14);
	idt_set_gate(47, (uint32_t) irq15);

	/* System call handler */
	idt_set_gate(255, (uint32_t) isr_sys);

	/* Program the PICs to remap IRQs to the range 32-47 */
	outb(PIC_MASTER_CMD, 0x11, true);
	outb(PIC_SLAVE_CMD, 0x11, true);
	outb(PIC_MASTER_DATA, 0x20, true);
	outb(PIC_SLAVE_DATA, 0x28, true);
	outb(PIC_MASTER_DATA, 0x04, true);
	outb(PIC_SLAVE_DATA, 0x02, true);
	outb(PIC_MASTER_DATA, 0x01, true);
	outb(PIC_SLAVE_DATA, 0x01, true);
	outb(PIC_MASTER_DATA, 0x00, true);
	outb(PIC_SLAVE_DATA, 0x00, true);

	/* Set up IDT pointer and call load_idt (in boot.s), which both loads
	   the IDT and enables interrupts */
	idt_ptr.limit = sizeof(idt) - 1;
	idt_ptr.base = (uint32_t) &idt;
	load_idt();
}

static void dump_exception(struct exception *e)
{
	kprintf("\nException %d (%x):\n", e->eno, e->err);
	kprintf("    EIP %x  PID %d\n", e->eip, current->pid);
	kprintf("    EAX %x  EBX %x  ECX %x  EDX %x\n",
	        e->eax, e->ebx, e->ecx, e->edx);
	kprintf("    ESI %x  EDI %x  EBP %x  ESP %x\n",
	        e->esi, e->edi, e->ebp, e->esp);
	kprintf("    EFL %x  CR0 %x  CR2 %x  CR3 %x\n",
	        e->eflags, e->cr0, e->cr2, e->cr3);
}

/* Main exception handler, which both catches processor exceptions and redirects
   interrupt requests to ISRs installed by drivers. */
void handle_exception(struct exception e)
{
	/* Handle system call */
	if (e.eno == INUM_SYSCALL) {
		handle_syscall(&e);
		return;
	}

	/* Call appropriate driver ISR (if installed) for IRQs */
	if (e.eno >= INUM_IRQ0 && e.eno <= INUM_IRQ15) {
		void (*handler)() = (void (*)()) 
		                    irq_handlers[e.eno - INUM_IRQ0];
		if (handler)
			handler();
		return;
	}
	
	/* Handle processor exceptions */
	switch (e.eno) {
	case INUM_DIVISION_BY_ZERO:
		if (kernel_exception(e)) {
			dump_exception(&e);
			kpanic("divide by zero exception");
		}
		else {
			current->state = TASK_NONE;
			kprintf("Divide by zero error: killed %d\n", current->pid);
			schedule();
			break;
		}

	case INUM_BREAKPOINT:
		if (kernel_exception(e)) {
			dump_exception(&e);
			kpanic("breakpoint exception");
		}

	case INUM_OUT_OF_BOUNDS:
		if (kernel_exception(e)) {
			dump_exception(&e);
			kpanic("out of bounds exception");
		}
		else {
			current->state = TASK_NONE;
			kprintf("Bounds error: killed %d\n", current->pid);
			schedule();
			break;
		}

	case INUM_INVALID_OPCODE:
		if (kernel_exception(e)) {
			dump_exception(&e);
			kpanic("invalid opcode exception");
		}
		else {
			current->state = TASK_NONE;
			kprintf("Invalid opcode: killed %d\n", current->pid);
			schedule();
			break;
		}

	case INUM_DOUBLE_FAULT:
		dump_exception(&e);
		kpanic("double fault exception");

	case INUM_STACK_FAULT:
		if (kernel_exception(e)) {
			dump_exception(&e);
			kpanic("stack fault exception");
		}

	case INUM_GENERAL_PROTECTION_FAULT:
		if (kernel_exception(e)) {
			dump_exception(&e);
			kpanic("general protection fault");
		}
		else {
			current->state = TASK_NONE;
			kprintf("General protection fault: killed %d\n", current->pid);
			schedule();
			break;
		}

	case INUM_PAGE_FAULT:
		if (kernel_exception(e)) {
			dump_exception(&e);
			kpanic("unexpected page fault");
		}
		else {
			/* To be potentially replaced by swapping someday... */
			current->state = TASK_NONE;
			kprintf("Page fault: killed %d\n", current->pid);
			schedule();
			break;
		}

	default:
		dump_exception(&e);
		kpanic("unhandled exception");
	}
}
