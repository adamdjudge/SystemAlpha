################################################################################
# Global struct for stashing the current registers during an exception, and also
# contains the exception number and error code. During a task switch, the
# contents of this struct are copied to the current task's process table entry
# and the next task's registers are copied in. This structure is also defined in
# interrupt.h for C usage.
################################################################################

.section .bss

.global except

except:
e_eax:
	.skip 4
e_ebx:
	.skip 4
e_ecx:
	.skip 4
e_edx:
	.skip 4
e_esi:
	.skip 4
e_edi:
	.skip 4
e_esp:
	.skip 4
e_ebp:
	.skip 4
e_eflags:
	.skip 4
e_eip:
	.skip 4
e_cs:
	.skip 4
e_ds:
	.skip 4
e_ss:
	.skip 4
e_es:
	.skip 4
e_fs:
	.skip 4
e_gs:
	.skip 4
e_cr0:
	.skip 4
e_cr2:
	.skip 4
e_cr3:
	.skip 4
e_eno:
	.skip 4
e_err:
	.skip 4

################################################################################
# Common exception handler, called by all the stubs for individual exception
# types below. At this point the stack contains, from SP upwards:
#
#     exception number, error code, EIP, CS, EFLAGS, (ESP, SS)
#
# where SS:ESP was only pushed if we came from user mode. We need to copy these,
# and all the other registers, into the global exception struct to be used by
# the C exception handler, and then copy them back to either return to the task
# that was interrupted, or a different task if a process switch occurs. However,
# we need to account for the fact that behavior is different depending on
# whether we're coming from/going to a user or kernel task.
################################################################################

.section .text

.extern handle_exception
.set KERNEL_CS, 0x8

isr_common:
        pop e_eno
        pop e_err
        pop e_eip
        pop e_cs
        pop e_eflags

        # If we came from the kernel task, ESP is now where it was before the
        # interrupt, so we should store it directly. Otherwise we store the user
        # task's SS:ESP, which is on the stack.

        cmp $KERNEL_CS, e_cs
        jne .from_user

.from_kernel:
        mov %esp, e_esp
        mov %ss, e_ss
        jmp .do_handler

.from_user:
        pop e_esp
        pop e_ss

        # Now we stash all the other registers and call the main exception
        # handling code, defined as a C function, and then restore everything.

.do_handler:
        mov %eax, e_eax
        mov %ebx, e_ebx
        mov %ecx, e_ecx
        mov %edx, e_edx
        mov %esi, e_esi
        mov %edi, e_edi
        mov %ebp, e_ebp
        mov %ds, e_ds
        mov %es, e_es
        mov %fs, e_fs
        mov %gs, e_gs

	mov %cr0, %eax
	mov %eax, e_cr0
	mov %cr2, %eax
	mov %eax, e_cr2
	mov %cr3, %eax
	mov %eax, e_cr3

        call handle_exception

        mov e_eax, %eax
        mov e_ebx, %ebx
        mov e_ecx, %ecx
        mov e_edx, %edx
        mov e_esi, %esi
        mov e_edi, %edi
        mov e_ebp, %ebp
        mov e_ds, %ds
        mov e_es, %es
        mov e_fs, %fs
        mov e_gs, %gs

        # Finally we need to set up the stack for an iret back to the task.
        # Depending on whether we're returning to a user or kernel task, we must
        # either push SS:ESP or set ESP manually.

        cmp $KERNEL_CS, e_cs
        jne .iret_user

.iret_kernel:
        mov e_esp, %esp
        push e_eflags
        push e_cs
        push e_eip
        iret

.iret_user:
        push e_ss
        push e_esp
        push e_eflags
        push e_cs
        push e_eip
        iret

################################################################################
# Start processor exception handlers. All of them push the interrupt number to
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

