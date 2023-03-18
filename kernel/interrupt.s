################################################################################
# Here we statically allocate the Interrupt Descriptor Table, which is an array
# of 256 structures defining an ISR vector for each possible interrupt. Much
# like with the GDT, we also need a descriptor to point to the IDT, which is
# loaded later on by the lidt instruction. After these we define a table of
# pointers to the ISR stubs defined below, which setup_idt loads into the IDT.
################################################################################

.section .data
.align 8

idt:
.rept 256
	.quad 0x00008e0000080000  # Flags=0x8e, segment=0x08, addrs set later
.endr

idt_desc:
	.word idt_desc - idt - 1  # IDT size - 1
	.long idt                 # IDT pointer

isr_table:
	.long isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,  isr8, isr9
	.long isr10, isr11, isr12, isr13, isr14, isr15, isr16, isr17, isr18
.rept 13
	.long ignore
.endr
	.long irq0, irq1, irq2,  irq3,  irq4,  irq5,  irq6,  irq7
	.long irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
.rept 207
	.long ignore
.endr
	.long isr_sys

################################################################################
# pic_remap programs the PICs to map the external interrupt requests to PIC
# interrupt numbers starting at 32, so as not to interfere with the interrupt
# numbers used by processor exceptions.
################################################################################

.section .text

.set PIC0_CMD, 0x20
.set PIC0_DATA, 0x21
.set PIC1_CMD, 0xA0
.set PIC1_DATA, 0xA1

pic_remap:
	mov $0x11, %al
	out %al, $PIC0_CMD
	out %al, $PIC1_CMD
	call pic_wait

	mov $0x20, %al
	out %al, $PIC0_DATA
	mov $0x28, %al
	out %al, $PIC1_DATA
	call pic_wait

	mov $0x04, %al
	out %al, $PIC0_DATA
	mov $0x02, %al
	out %al, $PIC1_DATA
	call pic_wait

	mov $0x01, %al
	out %al, $PIC0_DATA
	out %al, $PIC1_DATA
	call pic_wait

	mov $0x00, %al
	out %al, $PIC0_DATA
	out %al, $PIC1_DATA
	call pic_wait

	ret

pic_wait:
	mov $255, %cl
1:
	loop 1b
	ret

################################################################################
# setup_idt fills in the Interrupt Descriptor table with pointers to the
# interrupt service routine stubs defined below. The mapping of interrupt number
# to ISR is defined in isr_table above.
################################################################################

.global setup_idt

setup_idt:
	push %esi
	push %edi

	mov $isr_table, %esi
	mov $idt, %edi

1:
	mov (%esi), %eax
	add $4, %esi

	# Split up the ISR pointer into low and high words and store in the
	# appropriate fields of the IDT entry.
	mov %ax, (%edi)
	shr $16, %eax
	mov %ax, 6(%edi)
	add $8, %edi

	cmp $idt_desc, %edi
	jl 1b

	call pic_remap
	lidt idt_desc

	pop %edi
	pop %esi
	ret

# Unused IDT entries are set to point here to ignore unknown interrupts (though
# they shouldn't happen anyway) in isr_table.
ignore:
	iret

################################################################################
# Common exception handler, called by all the stubs for individual exception
# types below. At this point the stack contains, from SP upwards:
#
#     exception number, error code, EIP, CS, EFLAGS, (ESP, SS)
#
# where SS:ESP was only pushed if we came from user mode. We also need to push
# all the other registers to preserve task state, and then we can call the main
# exception handling code in C.
################################################################################

.extern handle_exception
.global iret_to_task

.set KERNEL_DS, 0x10
.set PIC_EOI, 0x20

isr_common:
	push %gs
	push %fs
	push %es
	push %ds
	pusha

	mov %cr3, %eax
	push %eax
	mov %cr2, %eax
	push %eax
	mov %cr0, %eax
	push %eax

	mov $KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

        call handle_exception

	# When a new task is created, its kernel stack is filled in so that when
	# it's scheduled for the first time, it returns to here, where it does
	# an iret to start running user code.
iret_to_task:
	add $12, %esp # Discard cr0, cr1, and cr3

	# If exception was an IRQ, send EOI command to the PIC(s).
	cmpl $32, 48(%esp)
	jl .skip_eoi
	cmpl $47, 48(%esp)
	jg .skip_eoi
	mov $PIC_EOI, %al
	cmpl $40, 48(%esp)
	jle .skip_slave_eoi
	out %al, $PIC1_CMD
.skip_slave_eoi:
	out %al, $PIC0_CMD

.skip_eoi:
	popa
	pop %ds
	pop %es
	pop %fs
	pop %gs
	add $8, %esp # Discard eno and err
	iret

################################################################################
# Start processor interrupt vectors. All of them push the interrupt number to
# the stack, which becomes exception.ino. Some also push a dummy error code
# before that, which becomes exception.err, in cases where the processor does
# not push an actual error code itself.
################################################################################

.global isr0
isr0:
	cli
	push $0
	push $0
	jmp isr_common

.global isr1
isr1:
	cli
	push $0
	push $1
	jmp isr_common

.global isr2
isr2:
	cli
	push $0
	push $2
	jmp isr_common

.global isr3
isr3:
	cli
	push $0
	push $3
	jmp isr_common

.global isr4
isr4:
	cli
	push $0
	push $4
	jmp isr_common

.global isr5
isr5:
	cli
	push $0
	push $5
	jmp isr_common

.global isr6
isr6:
	cli
	push $0
	push $6
	jmp isr_common

.global isr7
isr7:
	cli
	push $0
	push $7
	jmp isr_common

.global isr8
isr8:
	cli
	push $8
	jmp isr_common

.global isr9
isr9:
	cli
	push $0
	push $9
	jmp isr_common

.global isr10
isr10:
	cli
	push $10
	jmp isr_common

.global isr11
isr11:
	cli
	push $11
	jmp isr_common

.global isr12
isr12:
	cli
	push $12
	jmp isr_common

.global isr13
isr13:
	cli
	push $13
	jmp isr_common

.global isr14
isr14:
	cli
	push $14
	jmp isr_common

.global isr15
isr15:
	cli
	push $0
	push $15
	jmp isr_common

.global isr16
isr16:
	cli
	push $0
	push $16
	jmp isr_common

.global isr17
isr17:
	cli
	push $0
	push $17
	jmp isr_common

.global isr18
isr18:
	cli
	push $0
	push $18
	jmp isr_common

# Start external interrupt request handlers

.global irq0
irq0:
	cli
	push $0
	push $32
	jmp isr_common

.global irq1
irq1:
	cli
	push $0
	push $33
	jmp isr_common

.global irq2
irq2:
	cli
	push $0
	push $34
	jmp isr_common

.global irq3
irq3:
	cli
	push $0
	push $35
	jmp isr_common

.global irq4
irq4:
	cli
	push $0
	push $36
	jmp isr_common

.global irq5
irq5:
	cli
	push $0
	push $37
	jmp isr_common

.global irq6
irq6:
	cli
	push $0
	push $38
	jmp isr_common

.global irq7
irq7:
	cli
	push $0
	push $39
	jmp isr_common

.global irq8
irq8:
	cli
	push $0
	push $40
	jmp isr_common

.global irq9
irq9:
	cli
	push $0
	push $41
	jmp isr_common

.global irq10
irq10:
	cli
	push $0
	push $42
	jmp isr_common

.global irq11
irq11:
	cli
	push $0
	push $43
	jmp isr_common

.global irq12
irq12:
	cli
	push $0
	push $44
	jmp isr_common

.global irq13
irq13:
	cli
	push $0
	push $45
	jmp isr_common

.global irq14
irq14:
	cli
	push $0
	push $46
	jmp isr_common

.global irq15
irq15:
	cli
	push $0
	push $47
	jmp isr_common

# System call interrupt handler

.global isr_sys
isr_sys:
	cli
	push $0
	push $255
	jmp isr_common

