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
# we have dynamic memory allocation. First is the kernel stack, which is used
# during startup and then handed over to the system task, as well as a second
# stack that is switched to (via the TSS) when interrupting to kernel mode from
# a user task. We also need to create the initial page directory and page table
# so we can set up virtual memory paging.
################################################################################

.section .bss
.align 4096

.global kstack_top
.global istack_top
.global page_directory
.global page_table

kstack_bottom:
	.skip 4096
kstack_top:

istack_bottom:
	.skip 4096
istack_top:

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
	.long istack_top          # ESP0 (ESP for switch to ring 0)
	.long KERNEL_DS           # SS0 (SS for switch to ring 0)
	.skip 192

################################################################################
# Kernel entry point code. Sets the stack pointer, loads the GDT and TSS, sets
# the segment registers properly, and then calls the C main() function to fully
# initialize the kernel with a pointer to the info struct Multiboot gives us as
# the single argument. main() shouldn't return, but if it does we just lock up
# with an infinite loop.
################################################################################

.section .text

.extern main
.global start

start:
	cli
	mov $kstack_top, %esp

	# Set TSS segment base pointer here because stupid assembler won't let
	# us do logical operations to compute constants.
	mov $tss, %eax
	shl $16, %eax
	or %eax, gdt + KERNEL_TS
	mov $tss, %eax
	shr $16, %eax
	or %eax, gdt + KERNEL_TS + 4

	mov $KERNEL_DS, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	lgdt gdt_desc
	jmp $KERNEL_CS, $1f
1:
	mov $KERNEL_TS, %ax
	ltr %ax
	mov $tss, %eax

	push %ebx
	call main

1:
	hlt
	jmp 1b

################################################################################
# Miscellaneous assembly functions called externally.
################################################################################

.global enable_paging
.global set_cr3
.global flush_tlb
.global load_idt
.global _syscall

# Enable paging and virtual address translation
enable_paging:
	mov $page_directory, %eax
	mov %eax, %cr3
	mov %cr0, %eax
	or $0x80010000, %eax
	mov %eax, %cr0
	ret

# Set new CR3 value during task switch
set_cr3:
	mov 4(%esp), %eax
	mov %eax, %cr3
	ret

# Flush the TLB by reinstalling the same CR3 value
flush_tlb:
	mov %cr3, %eax
	mov %eax, %cr3
	ret

# Load the Interrupt Descriptor Table
.extern idt_ptr
load_idt:
	lidt idt_ptr
	ret

# Perform a system call from a kernel task
# int _syscall(in32_t header, in32_t *args)
_syscall:
	push %ebx
	push %esi
	push %edi

	mov 20(%esp), %eax
	mov 0(%eax), %ebx
	mov 4(%eax), %ecx
	mov 8(%eax), %edx
	mov 12(%eax), %esi
	mov 16(%eax), %edi

	mov 16(%esp), %eax
	int $255

	push %eax
	mov 24(%esp), %eax
	mov %ebx, 0(%eax)
	mov %ecx, 4(%eax)
	mov %edx, 8(%eax)
	mov %esi, 12(%eax)
	mov %edi, 16(%eax)

	pop %eax
	pop %edi
	pop %esi
	pop %ebx
	ret
