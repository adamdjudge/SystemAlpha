#include "types.h"
#include "util.h"

#include "paging.h"

/* Kernel starter page map, defined in boot.s */
extern uint32_t page_directory[];
extern uint32_t page_table[];

/* Defined in link.ld */
extern uint8_t kernel_code_end[];
extern uint8_t kernel_end[];

extern void enable_paging();

void paging_init()
{
	uint32_t i, addr;

	memset(page_directory, 0, PAGE_SIZE);
	memset(page_table, 0, PAGE_SIZE);

	for (i = 256; i < 1024; i++) {
		addr = i * PAGE_SIZE;
		if (addr > (uint32_t) kernel_end)
			break;

		/* Mark kernel code read-only, and everything else read-write */
		if (addr < (uint32_t) kernel_code_end)
			page_table[i] = addr | PAGE_PRESENT;
		else
			page_table[i] = addr | PAGE_PRESENT | PAGE_WRITABLE;
	}

	/* Map VGA text memory to 0xff000 */
	page_table[255] = 0xb8000 | PAGE_PRESENT | PAGE_WRITABLE;

	page_directory[0] = (uint32_t) page_table
			    | PAGE_PRESENT | PAGE_WRITABLE;

	enable_paging();
}
