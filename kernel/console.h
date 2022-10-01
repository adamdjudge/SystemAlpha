#ifndef CONSOLE_H
#define CONSOLE_H

#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 25
#define CONSOLE_TABSTOP 4

void console_init();
void console_clear();
void kprintf(char *fmt, ...);
void kpanic(char *msg);

#endif
