#ifndef CONSOLE_H
#define CONSOLE_H

#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 25
#define CONSOLE_TABSTOP 4
#define NUM_SCREENS 4
#define DEFAULT_COLOR 0x0f

void console_init();
void console_clear();
void putc(char c);
void put_char(int s, char c);
void switch_screen(int s);

#endif
