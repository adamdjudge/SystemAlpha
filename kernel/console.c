#include "io.h"
#include "types.h"
#include "util.h"

#include "console.h"

#define INDEX_REG 0x3d4
#define DATA_REG 0x3d5

#define POS_HIGH_INDEX 0xf
#define POS_LOW_INDEX 0xe
#define START_SCAN_INDEX 0xa
#define END_SCAN_INDEX 0xb

static char *text_mem = (char*) 0xff000;

struct {
	char buf[CONSOLE_HEIGHT*CONSOLE_WIDTH*2];
	char color;
	int pos;
} screens[NUM_SCREENS];
static int cur_screen;

static void update_cursor()
{
	outb(INDEX_REG, POS_HIGH_INDEX);
	outb(DATA_REG, screens[cur_screen].pos & 0xff);
	outb(INDEX_REG, POS_LOW_INDEX);
	outb(DATA_REG, (screens[cur_screen].pos >> 8) & 0xff);
}

void switch_screen(int s)
{
	if (s >= NUM_SCREENS)
		return;
	
	for (int i = 0; i < CONSOLE_HEIGHT*CONSOLE_WIDTH*2; i++)
		text_mem[i] = screens[s].buf[i];
	cur_screen = s;
	update_cursor();
}

void console_clear(int s)
{
	for (int i = 0; i < 2 * CONSOLE_WIDTH * CONSOLE_HEIGHT; i += 2) {
		screens[s].buf[i] = '\0';
		screens[s].buf[i+1] = DEFAULT_COLOR;
	}
	screens[s].color = DEFAULT_COLOR;
	screens[s].pos = 0;
}

void console_init()
{
	for (int s = 0; s < NUM_SCREENS; s++)
		console_clear(s);
	switch_screen(0);

	outb(INDEX_REG, START_SCAN_INDEX);
	outb(DATA_REG, inb(DATA_REG) & 0xc0);
	outb(INDEX_REG, END_SCAN_INDEX);
	outb(DATA_REG, (inb(DATA_REG) & 0xe0) | 15);
}

static void linefeed(int s)
{
	int line, i, bottom;
	char *dst, *src;

	for (line = 1; line < CONSOLE_HEIGHT; line++) {
		dst = screens[cur_screen].buf + (line-1) * 2*CONSOLE_WIDTH;
		src = screens[cur_screen].buf + line * 2*CONSOLE_WIDTH;
		memcpy(dst, src, 2*CONSOLE_WIDTH);

		if (s == cur_screen) {
			dst = text_mem + (line-1) * 2*CONSOLE_WIDTH;
			src = text_mem + line * 2*CONSOLE_WIDTH;
			memcpy(dst, src, 2*CONSOLE_WIDTH);
		}
	}

	bottom = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
	for (i = 0; i < CONSOLE_WIDTH; i++) {
		screens[cur_screen].buf[2*(bottom+i)] = '\0';
		if (s == cur_screen)
			text_mem[2*(bottom+i)] = '\0';
	}
}

static int write_screen(int s, char c)
{
	int pos = 2 * screens[cur_screen].pos;
	screens[cur_screen].buf[pos] = c;
	screens[cur_screen].buf[pos+1] = screens[cur_screen].color;
	if (s == cur_screen) {
		text_mem[pos] = c;
		text_mem[pos+1] = screens[cur_screen].color;
	}
}

static void put_char(int s, char c)
{
	int i;

	if (!c)
		return;
    
	switch (c) {
	case '\n':
		do {
			screens[s].pos++;
		} while (screens[s].pos % CONSOLE_WIDTH);
		break;
	case '\b':
		screens[s].pos--;
		break;
	case '\t':
		for (i = 0; i < CONSOLE_TABSTOP; i++)
			put_char(s, ' ');
		break;
	default:
		write_screen(s, c);
		screens[s].pos++;
	}

	if (screens[s].pos >= CONSOLE_WIDTH * CONSOLE_HEIGHT) {
		screens[s].pos = CONSOLE_WIDTH * (CONSOLE_HEIGHT - 1);
		linefeed(s);
	}
	if (s == cur_screen)
		update_cursor();
}

void putc(char c)
{
	put_char(cur_screen, c);
}
