#ifndef UTIL_H
#define UTIL_H

#include <kernel/types.h>

void memset(void *s, uint8_t c, uint32_t n);
void memcpy(void *dst, void *src, uint32_t n);
int str_eq(char *a, char *b);

#endif
