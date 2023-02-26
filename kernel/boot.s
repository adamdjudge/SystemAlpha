# Multiboot header, required by GRUB
.set MULTIBOOT_MAGIC, 0x1badb002
.set MULTIBOOT_FLAGS, 0x3
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC+MULTIBOOT_FLAGS)
.section .multiboot
	.long MULTIBOOT_MAGIC
	.long MULTIBOOT_FLAGS
	.long MULTIBOOT_CHECKSUM

# Static kernel stack
.section .bss
kstack_bottom:
.skip 16384
kstack_top:

# Static initial page directory and page table
.global page_directory
.global page_table
.align 4096
page_directory:
.skip 4096
page_table:
.skip 4096

# Kernel entry point
.section .text
.extern main
.global start
start:
	movl $kstack_top, %esp
	push %ebx
	call main
halt:
	hlt
	jmp halt

# Initialize the Global Descriptor Table
.extern gdt_ptr
.global load_gdt
load_gdt:
	lgdt gdt_ptr
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss
	jmp $0x08, $long_jmp
long_jmp:
	mov $0x28, %ax
	ltr %ax
	ret

# Enable paging and virtual address translation
.global enable_paging
enable_paging:
	movl $page_directory, %eax
	movl %eax, %cr3
	movl %cr0, %eax
	orl $0x80010000, %eax
	movl %eax, %cr0
	ret

# Flush the TLB after removing a page from the virtual mapping
.global flush_tlb
flush_tlb:
	movl %cr3, %eax
	movl %eax, %cr3
	ret

# Load the Interrupt Descriptor Table and enable interrupts
.extern idt_ptr
.global load_idt
load_idt:
	lidt idt_ptr
	sti
	ret

# Jump to user code execution after kernel initialization
.global jump_usermode
jump_usermode:
	mov $(0x20 | 3), %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %esp, %eax

	push $(0x20 | 3)          # user stack segment
	push %eax                 # user esp
	pushf                     # user flags
	push $(0x18 | 3)          # user code segment
	push $test_user_function  # user eip
	iret

test_user_function:
	cli
	push $0x0
	push $0xff

//==============================================================================
// Interrupt and Exception Routines
//==============================================================================

# Common interrupt handling code, which pushes an exception struct to the stack
# and calls the handle_interrupt function in idt.c
.extern handle_interrupt
isr_common:
	pusha
	push %ss
	push %ds
	push %es
	push %fs
	push %gs
	
	movw $0x10, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	mov $handle_interrupt, %eax
	call *%eax

	pop %eax
	pop %gs
	pop %fs
	pop %es
	pop %ds
	popa
	add $8, %esp
	iret

# Start processor exception handlers. All of them push the interrupt number to
# the stack, which becomes exception.inum. Some also push a dummy error code
# before that, which becomes exception.ecode, in cases where the processor
# does not push an actual error code itself.

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
