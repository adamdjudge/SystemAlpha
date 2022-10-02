#ifndef PAGING_H
#define PAGING_H

#define PAGE_SIZE 4096

#define PAGE_PRESENT   1<<0
#define PAGE_WRITABLE  1<<1
#define PAGE_USER      1<<2

void paging_init();

#endif
