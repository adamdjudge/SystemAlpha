#include "types.h"
#include "util.h"
#include "console.h"
#include "sched.h"
#include "malloc.h"

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

	/* We may need to allocate a new page table within the page directory
	   in order to setup the requested virtual address. */
	if (!(pdir[dirent] & PAGE_PRESENT)) {
		paddr = pop_page();
		if (!paddr)
			return 0;
		pdir[dirent] = paddr | PAGE_PRESENT | PAGE_WRITABLE | flags;
	}

	/* Now we can set the page table entry. */
	paddr = pop_page();
	if (!paddr)
		return 0;
	ptab[tabent] = paddr | PAGE_PRESENT | flags;
	return paddr;
}

uint32_t alloc_kernel_page(uint32_t flags)
{
	static uint32_t vaddr = 0x800000;
	if (alloc_page(vaddr, flags)) {
		vaddr += PAGE_SIZE;
		return vaddr - PAGE_SIZE;
	}
	return 0;
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

uint32_t vtophys(uint32_t vaddr)
{
	int dirent = (vaddr >> 22) & 0x3ff;
	int tabent = (vaddr >> 12) & 0x3ff;

	uint32_t *pdir = (uint32_t*) 0x401000;
	uint32_t *ptab = (uint32_t*) (0x400000 + dirent * PAGE_SIZE);

	if (!(pdir[dirent] & PAGE_PRESENT) || !(ptab[tabent] & PAGE_PRESENT))
		return 0;
	
	return (ptab[tabent] & ~0xfff) | (vaddr & 0xfff);
}

/*
 * Allocates a page for use by a user process at the specified address within
 * that process's virtual address space. This page is also mapped into kernel
 * space so that the kernel can access it.
 */
uint32_t alloc_user_page(struct task *t, uint32_t uvaddr)
{
	struct user_page *newpg, *newtab;
	int dirent = (uvaddr >> 22) & 0x3ff;
	int tabent = (uvaddr >> 12) & 0x3ff;
	uint32_t tabpage;

	if (!(t->page_dir[dirent] & PAGE_PRESENT)) {
		newtab = kmalloc(sizeof(struct user_page), 0);
		if (!newtab)
			return 0;
		
		tabpage = alloc_kernel_page(PAGE_WRITABLE);
		if (!tabpage)
			return 0;
		t->page_dir[dirent] = vtophys(tabpage) | PAGE_PRESENT
		                      | PAGE_WRITABLE | PAGE_USER;
		
		newtab->kvaddr = tabpage;
		newtab->next = t->page_tables;
		t->page_tables = newtab;
	}
	else {
		newtab = t->page_tables;
		while (vtophys(newtab->kvaddr) != (t->page_dir[dirent] & ~0xfff))
			newtab = newtab->next;
		tabpage = newtab->kvaddr;
	}
	
	newpg = kmalloc(sizeof(struct user_page), 0);
	if (!newpg)
		return 0;
	
	newpg->kvaddr = alloc_kernel_page(PAGE_USER | PAGE_WRITABLE);
	if (!newpg->kvaddr)
		return 0;
	
	newpg->uvaddr = uvaddr;
	((uint32_t*) tabpage)[tabent] = vtophys(newpg->kvaddr) | PAGE_PRESENT
	                                | PAGE_WRITABLE | PAGE_USER;
	newpg->next = t->pages;
	t->pages = newpg;

	if (t == current)
		flush_tlb();
	return newpg->kvaddr;
}
