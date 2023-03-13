#include "kernel.h"
#include "util.h"
#include "paging.h"
#include "console.h"
#include "keyboard.h"
#include "idt.h"
#include "malloc.h"
#include "sched.h"
#include "syscall.h"

static void printd(uint32_t val);
static void printx16(uint32_t val);
static void printx32(uint32_t val);
static void prints(char *s);

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
	keyboard_init();
	heap_init();
	sched_init();

	kprintf("System Alpha kernel v0.0.1\n");
	kprintf("(C) 2023 Adam Judge\n");

	kprintf("Upper memory: %dk\n", mem_upper);
	if (mem_upper < 1024)
		kpanic("upper memory size less than 1024k");

	spawn_kernel_task(sender_task);
	spawn_kernel_task(receiver_task);
	idle_task();
}

void kprintf(char *fmt, ...)
{
	char *c;
	uint32_t *argptr = (uint32_t*) &fmt + 1;

	for (c = fmt; *c != '\0'; c++) {
		if (*c != '%') {
			putc(*c);
			continue;
		}

		switch (*(++c)) {
		case '%':
			putc('%');
			break;
		case 'd':
			printd(*(argptr++));
			break;
		case 'w':
			printx16(*(argptr++));
			break;
		case 'x':
			printx32(*(argptr++));
			break;
		case 's':
			prints((char*) *(argptr++));
			break;
		case 'c':
			putc((char) *(argptr++));
			break;
		default:
			putc('%');
			putc(*c);
		}
	}
}

void kpanic(char *msg)
{
	asm("cli");
	kprintf("Kernel panic: %s", msg);
	for (;;);
}

static void printd(uint32_t val)
{
	if (val / 10)
		printd(val / 10);
	putc('0' + val % 10);
}

static void printx16(uint32_t val)
{
	int i, digit;

	for (i = 12; i >= 0; i -= 4) {
		digit = (val >> i) & 0xf;
		if (digit < 10)
			putc('0' + digit);
		else
			putc('a' + digit-10);
	}
}

static void printx32(uint32_t val)
{
	int i, digit;

	for (i = 28; i >= 0; i -= 4) {
		digit = (val >> i) & 0xf;
		if (digit < 10)
			putc('0' + digit);
		else
			putc('a' + digit-10);
	}
}

static void prints(char *s)
{
	while (*s)
		putc(*(s++));
}
