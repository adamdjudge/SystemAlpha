#ifndef IO_H
#define IO_H

#include "types.h"

static inline uint8_t inb(uint16_t port)
{
	uint8_t data;
	asm("inb %1, %0" : "=a" (data) : "Nd" (port));
	return data;
}

static inline void outb(uint16_t port, uint8_t data)
{
	asm("outb %0, %1" : : "a" (data), "Nd" (port));
}

#endif
