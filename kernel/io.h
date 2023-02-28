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

static inline uint16_t inw(uint16_t port)
{
	uint16_t data;
	asm("inw %1, %0" : "=a" (data) : "Nd" (port));
	return data;
}

static inline void outw(uint16_t port, uint16_t data)
{
	asm("outw %0, %1" : : "a" (data), "Nd" (port));
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t data;
	asm("inl %1, %0" : "=a" (data) : "Nd" (port));
	return data;
}

static inline void outl(uint16_t port, uint32_t data)
{
	asm("outl %0, %1" : : "a" (data), "Nd" (port));
}

#endif
