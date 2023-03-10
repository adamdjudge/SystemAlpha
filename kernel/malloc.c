#include "util.h"
#include "paging.h"
#include "console.h"

#include "malloc.h"

#define CHUNK_HEADER     0x80000000
#define CHUNK_ALLOCATED  0x40000000
#define SIZE_MASK        0x00ffffff

static uint32_t *heap = NULL;
static uint32_t *limit;

void heap_init()
{
        uint32_t i, page;

        for (i = 0; i < HEAP_PAGES; i++) {
                page = alloc_kernel_page(PAGE_WRITABLE);
                if (!page)
                        kpanic("heap allocation failed");
                else if (!heap)
                        heap = (uint32_t*) page;
        }

        *heap = (HEAP_PAGES * PAGE_SIZE + 1) | CHUNK_HEADER;
        limit = heap + (HEAP_PAGES * PAGE_SIZE / 4);
}

void *kmalloc(size_t size, uint32_t flags)
{
        uint32_t *ptr, chunk_size;
        
        /* Convert size to number of dwords, rounded up */
        size = ((size + 3) & ~3) >> 2;
        if (size & ~SIZE_MASK)
                return NULL;

        for (ptr = heap; ptr < limit; ptr += *ptr & SIZE_MASK) {
                if (!(*ptr & CHUNK_HEADER))
                        kpanic("heap corrupted");

                if (!(*ptr & CHUNK_ALLOCATED)) {
                        chunk_size = *ptr & SIZE_MASK;
                        if (chunk_size - 1 < size)
                                continue;
                        if (ptr + size >= limit)
                                return NULL;

                        *ptr = (size + 1) | CHUNK_HEADER | CHUNK_ALLOCATED;
                        *(ptr + size + 1) = (chunk_size - size) | CHUNK_HEADER;
                        return ptr + 1;
                }
        }
        return NULL;
}

void kfree(void *ptr)
{
        return;
}
