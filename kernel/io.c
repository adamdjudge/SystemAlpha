#include <asm/io.h>

uint8_t inb(uint16_t port, bool wait)
{
	uint8_t data;
	asm("inb %1, %0" : "=a" (data) : "Nd" (port));
        if (wait)
                for (int i = 0; i < 255; i++);
	return data;
}

void outb(uint16_t port, uint8_t data, bool wait)
{
	asm("outb %0, %1" : : "a" (data), "Nd" (port));
        if (wait)
                for (int i = 0; i < 255; i++);
}

uint16_t inw(uint16_t port, bool wait)
{
	uint16_t data;
	asm("inw %1, %0" : "=a" (data) : "Nd" (port));
        if (wait)
                for (int i = 0; i < 255; i++);
	return data;
}

void outw(uint16_t port, uint16_t data, bool wait)
{
	asm("outw %0, %1" : : "a" (data), "Nd" (port));
        if (wait)
                for (int i = 0; i < 255; i++);
}

uint32_t inl(uint16_t port, bool wait)
{
	uint32_t data;
	asm("inl %1, %0" : "=a" (data) : "Nd" (port));
        if (wait)
                for (int i = 0; i < 255; i++);
	return data;
}

void outl(uint16_t port, uint32_t data, bool wait)
{
	asm("outl %0, %1" : : "a" (data), "Nd" (port));
        if (wait)
                for (int i = 0; i < 255; i++);
}
