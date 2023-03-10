#ifndef MALLOC_H
#define MALLOC_H

#include "types.h"

/* Number of pages to allocate for use as kernel heap */
#define HEAP_PAGES 4

void heap_init();

void *kmalloc(size_t size, uint32_t flags);

void kfree(void *ptr);

#endif
