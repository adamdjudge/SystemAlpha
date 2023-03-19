#ifndef PAGING_H
#define PAGING_H

#include <kernel/types.h>
#include <kernel/sched.h>

#define PAGE_SIZE 4096

#define PAGE_PRESENT   (1<<0)
#define PAGE_WRITABLE  (1<<1)
#define PAGE_USER      (1<<2)

void paging_init();
uint32_t alloc_page(uint32_t vaddr, uint32_t flags);
uint32_t alloc_kernel_page(uint32_t flags);
void free_page(uint32_t vaddr);
uint32_t vtophys(uint32_t vaddr);
uint32_t alloc_user_page(struct task *t, uint32_t uvaddr);

#endif
