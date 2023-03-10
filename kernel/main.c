#include "types.h"
#include "util.h"
#include "paging.h"
#include "console.h"
#include "idt.h"
#include "malloc.h"
#include "sched.h"

void system_task()
{
        asm("sti");
        for (;;) {
                kprintf("hello from the system task\n");
                for (int i = 0; i < 0x8ffffff; i++);
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
	
	struct task *t = spawn_user_process();
	uint8_t *m = (uint8_t*) alloc_user_page(t, 0x80000000);
	m[0] = 0x90;
	m[1] = 0x90;
	m[2] = 0xfa;

	system_task();
}
