#include "types.h"
#include "util.h"
#include "console.h"

#include "paging.h"

#define PAGE_ALIGN(n) ((n + 0xfff) & ~0xfff)

/* Free page stack, used to allocate physical pages */
static uint32_t page_stack[4096];
static uint32_t stackp;

static inline uint32_t pop_page()
{
	return stackp > 0 ? page_stack[--stackp] : 0;
}

static inline void push_page(uint32_t addr)
{
	page_stack[stackp++] = addr;
}

/* Kernel starter page map, defined in boot.s */
extern uint32_t page_directory[];
extern uint32_t page_table[];

/* Defined in link.ld */
extern uint8_t kernel_code_end[];
extern uint8_t kernel_end[];

/* Defined in boot.s */
extern void enable_paging();
extern void flush_tlb();

/* 
 * Initializes the starter kernel page map, then fills the free page stack with
 * available physical pages after the kernel up to at most mem_upper, the
 * boundary in KiB of the upper memory region given by multiboot.
 */
void paging_init(uint32_t mem_upper)
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
	page_directory[1] = (uint32_t) page_directory
			    | PAGE_PRESENT | PAGE_WRITABLE;

	enable_paging();

	/* Fill free page stack */
	stackp = 0;
	for (i = 0; i < sizeof(page_stack); i++) {
		if (i == mem_upper)
			break;
		
		addr = PAGE_ALIGN((uint32_t) kernel_end) + i * 1024;
		if (addr % PAGE_SIZE == 0)
			push_page(addr);
	}
}

uint32_t alloc_page(uint32_t vaddr, uint32_t flags)
{
	int dirent = (vaddr >> 22) & 0x3ff;
	int tabent = (vaddr >> 12) & 0x3ff;

	uint32_t *pdir = (uint32_t*) 0x401000;
	uint32_t *ptab = (uint32_t*) (0x400000 + dirent * PAGE_SIZE);
	uint32_t paddr;

	if (!(pdir[dirent] & PAGE_PRESENT)) {
		paddr = pop_page();
		if (!paddr)
			return 0;
		pdir[dirent] = paddr | PAGE_PRESENT | PAGE_WRITABLE;
	}

	paddr = pop_page();
	if (!paddr)
		return 0;
	ptab[tabent] = paddr | PAGE_PRESENT | flags;
	return paddr;
}

void free_page(uint32_t vaddr)
{
	int dirent = (vaddr >> 22) & 0x3ff;
	int tabent = (vaddr >> 12) & 0x3ff;

	uint32_t *pdir = (uint32_t*) 0x401000;
	uint32_t *ptab = (uint32_t*) (0x400000 + dirent * PAGE_SIZE);
	uint32_t paddr;

	if (!(pdir[dirent] & PAGE_PRESENT) || !(ptab[tabent] & PAGE_PRESENT))
		kpanic("tried to free unallocated page!");

	paddr = ptab[tabent] & ~0xfff;
	ptab[tabent] = 0;
	push_page(paddr);
	flush_tlb();
}
