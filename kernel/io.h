#ifndef IO_H
#define IO_H

#include "types.h"

uint8_t inb(uint16_t port, bool wait);
void outb(uint16_t port, uint8_t data, bool wait);
uint16_t inw(uint16_t port, bool wait);
void outw(uint16_t port, uint16_t data, bool wait);
uint32_t inl(uint16_t port, bool wait);
void outl(uint16_t port, uint32_t data, bool wait);

#endif
