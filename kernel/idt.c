#include "kernel.h"
#include "io.h"
#include "sched.h"
#include "syscall.h"

#include "interrupt.h"
#include "idt.h"

/* Array of IRQ handlers installable by drivers */
static uint32_t irq_handlers[16] = {0};

/* Install an IRQ handler to be called whenever that IRQ number is fired */
void idt_install_isr(uint8_t irq_num, void (*handler)())
{
	if (irq_num > 15)
		return;
	irq_handlers[irq_num] = (uint32_t) handler;
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
