#include "types.h"
#include "util.h"
#include "paging.h"
#include "console.h"
#include "idt.h"
#include "malloc.h"
#include "sched.h"
#include "syscall.h"

void sender_task()
{
	struct syscall_args args;
	int i = 1, ret;

	for (;;) {
		sys_sleep(1000);
		args._1 = i++;

		ret = sys_send(2, &args);
		if (ret < 0) {
			kprintf("send failed: %d\n", -ret);
			for (;;);
		}
	}
}

void receiver_task()
{
	struct syscall_args args;
	int pid;

	for (;;) {
		pid = sys_recv(&args);
		kprintf("message from pid %d: %d\n", pid, args._1);
	}
}

void main(const uint32_t *multiboot_info)
{
	uint32_t mem_upper = multiboot_info[2];

	paging_init(mem_upper);
	console_init();
	idt_init();
	heap_init();
	sched_init();

	kprintf("System Alpha kernel v0.0.1\n");
	kprintf("(C) 2023 Adam Judge\n");

	kprintf("Upper memory: %dk\n", mem_upper);
	if (mem_upper < 4096)
		kpanic("upper memory size less than 4096k");

	spawn_kernel_task(sender_task);
	spawn_kernel_task(receiver_task);
	idle_task();
}
