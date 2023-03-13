#include "kernel.h"
#include "io.h"
#include "idt.h"
#include "console.h"

/* I/O ports */
#define PS2_DATA 0x60
#define PS2_CMD 0x64
#define PS2_STATUS 0x64

/* PS2_STATUS bits */
#define STATUS_OUT_FULL 0x1
#define STATUS_IN_FULL 0x2

/* PS2_CMD values */
#define PS2CMD_ENABLE_1 0xae
#define PS2CMD_DISABLE_1 0xad
#define PS2CMD_ENABLE_2 0xa8
#define PS2CMD_DISABLE_2 0xa7
#define PS2CMD_READ_CONFIG 0x20
#define PS2CMD_WRITE_CONFIG 0x60

/* Keyboard device commands */
#define KBDCMD_ENABLE 0xf4
#define KBDCMD_DISABLE 0xf5
#define KBDCMD_SETLED 0xed
#define KBDCMD_ECHO 0xee
#define KBDCMD_IDENTIFY 0xf2
#define KBDCMD_SELFTEST 0xff

/* Special keycodes */

#define KEY_RELEASE 0x80

#define KEY_LSHIFT 0x2a
#define KEY_RSHIFT 0x36
#define KEY_CTRL 0x1d
#define KEY_ALT 0x38

#define KEY_F1 0x3b
#define KEY_F2 0x3c
#define KEY_F3 0x3d
#define KEY_F4 0x3e
#define KEY_F5 0x3f
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_F11 0x57
#define KEY_F12 0x58

/* Maps scancode to ASCII when shift is not pressed. */
static const char noshift_map[] = {
/*       x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF */
/* 0x */ 0x0, 0x0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
/* 1x */ 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0x0, 'a', 's',
/* 2x */ 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0x0, '\\', 'z', 'x', 'c', 'v',
/* 3x */ 'b', 'n', 'm', ',', '.', '/', 0x0, '*', 0x0, ' ', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 4x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
/* 5x */ '2', '3', '0', '.', 0x0, ' ', ' ', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 6x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 7x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 8x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 9x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Ax */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Bx */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Cx */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Dx */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Ex */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Fx */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
};

/* Maps scancode to ASCII when shift is pressed. */
static const char shift_map[] = {
/*       x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF */
/* 0x */ 0x0, 0x0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
/* 1x */ 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0x0, 'A', 'S',
/* 2x */ 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0x0, '|', 'Z', 'X', 'C', 'V',
/* 3x */ 'B', 'N', 'M', '<', '>', '?', 0x0, 0x0, 0x0, ' ', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 4x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
/* 5x */ '2', '3', '0', '.', 0x0, ' ', ' ', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 6x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 7x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 8x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* 9x */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Ax */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Bx */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Cx */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Dx */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Ex */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
/* Fx */ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
};

/* Last decoded key pressed */
static char key;

/* Inverted every keypress, used to detect new key */
static bool phase = false;

/* Modifiers */
static bool shift = false;
static bool ctrl = false;
static bool alt = false;

static void controller_cmd(uint8_t cmd)
{
	while (inb(PS2_STATUS, false) & STATUS_IN_FULL);
	outb(PS2_CMD, cmd, false);
}

static void keyboard_cmd(uint8_t cmd)
{
	while (inb(PS2_STATUS, false) & STATUS_IN_FULL);
	outb(PS2_DATA, cmd, false);
}

static uint8_t read_data()
{
	while ((inb(PS2_STATUS, false) & STATUS_OUT_FULL) == 0);
	return inb(PS2_DATA, false);
}

static void keyboard_handler()
{
	unsigned char data = inb(PS2_DATA, false);

	if (data >= KEY_F1 && data <= KEY_F9 && ctrl && alt)
		switch_screen(data - KEY_F1);

	else if (data == KEY_LSHIFT || data == KEY_RSHIFT)
		shift = true;
	else if (data == (KEY_LSHIFT | KEY_RELEASE) || data == (KEY_RSHIFT | KEY_RELEASE))
		shift = false;
	else if (data == KEY_CTRL)
		ctrl = true;
	else if (data == (KEY_CTRL | KEY_RELEASE))
		ctrl = false;
	else if (data == KEY_ALT)
		alt = true;
	else if (data == (KEY_ALT | KEY_RELEASE))
		alt = false;

	else if ((data & KEY_RELEASE) == 0) {
		key = shift ? shift_map[data] : noshift_map[data];
		phase = !phase;
	}
}

/* Waits until next ASCII key is pressed and returns it. */
char getc()
{
	bool p = phase;
	while (phase == p);
	return key;
}

void keyboard_init()
{
	uint8_t config;

	/* Disable devices */
	controller_cmd(PS2CMD_DISABLE_1);
	controller_cmd(PS2CMD_DISABLE_2);

	/* Flush output buffer */
	inb(PS2_DATA, true);

	/* Get and reset config byte */
	controller_cmd(PS2CMD_READ_CONFIG);
	config = read_data();
	controller_cmd(PS2CMD_WRITE_CONFIG);
	outb(PS2_DATA, config | 0x1, false);

	/* Enable device and scanning */
	controller_cmd(PS2CMD_ENABLE_1);
	keyboard_cmd(KBDCMD_ENABLE);

	idt_install_isr(1, keyboard_handler);
}