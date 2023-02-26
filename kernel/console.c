#include "io.h"
#include "types.h"
#include "util.h"
#include "timer.h"

#include "console.h"

#define INDEX_REG 0x3d4
#define DATA_REG 0x3d5

#define POS_HIGH_INDEX 0xf
#define POS_LOW_INDEX 0xe
#define START_SCAN_INDEX 0xa
#define END_SCAN_INDEX 0xb

static char *text_mem = (char*) 0xff000;
static uint32_t cursor_pos;

static void update_cursor()
{
	outb(INDEX_REG, POS_HIGH_INDEX);
	outb(DATA_REG, cursor_pos & 0xff);
	outb(INDEX_REG, POS_LOW_INDEX);
	outb(DATA_REG, (cursor_pos >> 8) & 0xff);
}

void console_clear()
{
	int i;

	for (i = 0; i < 2 * CONSOLE_WIDTH * CONSOLE_HEIGHT; i += 2) {
		text_mem[i] = '\0';
		text_mem[i+1] = 0x1f;
	}
	cursor_pos = 0;
	update_cursor();
}

void console_init()
{
	console_clear();
	outb(INDEX_REG, START_SCAN_INDEX);
	outb(DATA_REG, inb(DATA_REG) & 0xc0);
	outb(INDEX_REG, END_SCAN_INDEX);
	outb(DATA_REG, (inb(DATA_REG) & 0xe0) | 15);
}

static void linefeed()
{
	int line, i;
	void *dst, *src;

	for (line = 1; line < CONSOLE_HEIGHT; line++) {
		dst = (void*) text_mem + (line-1) * 2*CONSOLE_WIDTH;
		src = (void*) text_mem + line * 2*CONSOLE_WIDTH;
		memcpy(dst, src, 2*CONSOLE_WIDTH);
	}

	for (i = 0; i < CONSOLE_WIDTH; i++)
		text_mem[2 * (((CONSOLE_HEIGHT-1) * CONSOLE_WIDTH) + i)] = '\0';
}

static void putc(char c)
{
	int i;

	if (!c)
		return;
    
	switch (c) {
	case '\n':
		do {
			cursor_pos++;
		} while (cursor_pos % CONSOLE_WIDTH);
		break;
	case '\b':
		cursor_pos--;
		break;
	case '\t':
		for (i = 0; i < CONSOLE_TABSTOP; i++)
			putc(' ');
		break;
	default:
		text_mem[2*cursor_pos] = c;
		cursor_pos++;
	}

	if (cursor_pos >= CONSOLE_WIDTH * CONSOLE_HEIGHT) {
		cursor_pos = CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1);
		linefeed();
	}

	update_cursor();
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

void kprintf(char *fmt, ...)
{
	char *c;
	uint32_t *argptr = (uint32_t*) &fmt + 1;

	putc('[');
	printd(jiffies());
	putc(']');
	putc(' ');

	for (c = fmt; *c != '\0'; c++) {
		if (*c != '%') {
			putc(*c);
			continue;
		}

		switch (*(++c)) {
		case '\0':
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
			prints((char*) *(argptr--));
			break;
		default:
			putc('%');
			putc(*c);
		}
	}
}

void kpanic(char *msg)
{
	kprintf("kernel panic: %s", msg);
	for (;;);
}
