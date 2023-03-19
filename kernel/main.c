#include "kernel.h"
#include "util.h"
#include "paging.h"
#include "console.h"
#include "keyboard.h"
#include "malloc.h"
#include "sched.h"
#include "syscall.h"

#include "../drivers/tty.h"

static void printd(uint32_t val);
static void printx16(uint32_t val);
static void printx32(uint32_t val);
static void prints(char *s);

void test1()
{
	for (;;) {
		kprintf("first task\n");
		for (int i = 0; i < 100000000; i++);
	}
}

void test2()
{
	for (;;) {
		kprintf("second task\n");
		for (int i = 0; i < 200000000; i++);
	}
}

void main(const uint32_t *multiboot_info)
{
	uint32_t mem_upper = multiboot_info[2];

	paging_init(mem_upper);
	console_init();
	keyboard_init();
	heap_init();
	sched_init();

	kprintf("System Alpha kernel v0.0.1\n");
	kprintf("(C) 2023 Adam Judge\n");

	kprintf("Upper memory: %dk\n", mem_upper);
	if (mem_upper < 1024)
		kpanic("upper memory size less than 1024k");

	tty_init();

	spawn_kthread(test1);
	spawn_kthread(test2);

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
