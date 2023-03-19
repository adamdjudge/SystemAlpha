################################################################################
# The Multiboot header, which is placed by the linker at the very beginning of
# the compiled executable's text segment and tells GRUB that this is a valid
# bootable kernel.
################################################################################

.set MULTIBOOT_MAGIC, 0x1badb002
.set MULTIBOOT_FLAGS, 0x3
.set MULTIBOOT_CHECKSUM, -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

.section .multiboot
	.long MULTIBOOT_MAGIC
	.long MULTIBOOT_FLAGS
	.long MULTIBOOT_CHECKSUM

################################################################################
# Here we statically define a few buffers used in kernel initialization before
# we have dynamic memory allocation. These are the kernel stack, which is used
# during startup, and the initial page directory and page table, needed to set
# up virtual memory paging.
################################################################################

.section .bss
.align 4096

.global kstack_top
.global page_directory
.global page_table

kstack_bottom:
	.skip 4096
kstack_top:

page_directory:
	.skip 4096

page_table:
	.skip 4096

################################################################################
# One of the peculiarities of x86 is the Global Descriptor Table, which defines
# segments. We need to create segments for code and data for both kernel and
# user access, but since we just use a flat memory model, each of these segments
# is configured to span the entire address range. We also need to define a Task
# State Segment (TSS), which we don't use to its full extent, but it is still
# required as it contains values for SS:ESP to switch to when interrupting from
# user to kernel mode. The GDT is pointed to by a GDT descriptor, which we later
# use with the lgdt instruction to load the GDT.
#
# We could set up the GDT the fancy way in C using structs and lots of bit
# manipulations and having a function to install entries properly, but since
# it's an ugly x86-specific feature, we'll just keep it here in assembly, and
# also just define it with constants for simplicity. Fortunately, once it and
# the TSS are set up they never have to be modified again.
################################################################################

.section .data
.align 8

.set KERNEL_CS, 0x8
.set KERNEL_DS, 0x10
.set KERNEL_TS, 0x28

gdt:
	.quad 0x0000000000000000  # Null segment
	.quad 0x00cf9a000000ffff  # Kernel code
	.quad 0x00cf92000000ffff  # Kernel data
	.quad 0x00cffa000000ffff  # User code
	.quad 0x00cff2000000ffff  # User data
	.quad 0x0000890000000068  # TSS (base address field set later)

gdt_desc:
	.word gdt_desc - gdt - 1  # Size of GDT - 1
	.long gdt                 # GDT pointer

.align 4
tss:
	.long 0                   # Reserved
	.long 0                   # ESP0, set when task switching
	.long KERNEL_DS           # SS0
	.skip 192

################################################################################
# Kernel entry point code. Sets the stack pointer and segment registers, loads
# the GDT and TSS, calls external code to setup the IDT, and then calls the C
# main() function to fully initialize the kernel, with a pointer to the info
# struct Multiboot gives us as the single argument. main() should not return,
# but if it does we just lock up with an infinite loop.
################################################################################

.section .text

.extern setup_idt
.extern main
.global start

start:
	cli
	mov $kstack_top, %esp

	# Set TSS segment base pointer here because the assembler won't let us
	# do logical operations to compute constants. This simple method is fine
	# because we know the address will have the highest 8 bits cleared.
	mov $tss, %eax
	or %eax, gdt + KERNEL_TS + 2

	mov $KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	lgdt gdt_desc
	jmp $KERNEL_CS, $1f
1:	mov $KERNEL_TS, %ax
	ltr %ax
	mov $tss, %eax

	call setup_idt

	push %ebx
	call main

1:	hlt
	jmp 1b

################################################################################
# Miscellaneous assembly functions called externally.
################################################################################

.global enable_paging
.global flush_tlb
.global load_idt
.global switch_task

# Enable paging and virtual address translation.
enable_paging:
	mov $page_directory, %eax
	mov %eax, %cr3
	mov %cr0, %eax
	or $0x80010000, %eax
	mov %eax, %cr0
	ret

# Flush the TLB by reinstalling the same CR3 value.
flush_tlb:
	mov %cr3, %eax
	mov %eax, %cr3
	ret

# Performs the switch to the next task by swapping the current kernel stack,
# TSS.ESP0, CR3, and the global current task pointer.
.extern current
.extern _next
switch_task:
	pusha
	mov current, %edi
	mov _next, %esi

	mov %esp, 0(%edi)
	mov 0(%esi), %esp
	mov 4(%esi), %eax
	mov %eax, tss+4
	mov 8(%esi), %eax
	mov %eax, %cr3

	mov %esi, current
	popa
	ret
