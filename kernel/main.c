#include "types.h"
#include "util.h"
#include "paging.h"
#include "console.h"
#include "idt.h"

static void gdt_init();

extern void jump_usermode();

void main(const uint32_t *multiboot_info)
{
	uint32_t mem_upper = multiboot_info[2];

	gdt_init();
	paging_init(mem_upper);
	console_init();
	idt_init();

	kprintf("System Alpha kernel v0.0.1\n");
	kprintf("(C) 2022 Adam Judge\n");

	kprintf("Upper memory: %dk\n", mem_upper);
	if (mem_upper < 4096)
		kpanic("upper memory size less than 4096k");
}

//==============================================================================
// Global Descriptor Table Setup
//==============================================================================

struct gdt_entry {
	uint16_t limit_low;
	uint32_t base_low: 24;
	uint32_t access: 8;
	uint8_t limit_high: 4;
	uint8_t flags: 4;
	uint8_t base_high;
} __attribute__((packed));

struct gdt_entry gdt[6];

struct {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) gdt_ptr;

static void set_gdt_entry(uint32_t i, uint32_t base, uint32_t limit,
			  uint8_t access, uint8_t flags)
{
	gdt[i].base_low = base & 0xffffff;
	gdt[i].base_high = (base & 0xff) >> 24;
	gdt[i].limit_low = limit & 0xffff;
	gdt[i].limit_high = (limit & 0xf) >> 16;
	gdt[i].access = access;
	gdt[i].flags = flags & 0xf;
}

extern void load_gdt();

uint32_t tss[26];

static void gdt_init()
{
	gdt_ptr.limit = sizeof(gdt) - 1;
	gdt_ptr.base = (uint32_t) &gdt;
	set_gdt_entry(0, 0, 0, 0, 0);
	set_gdt_entry(1, 0, 0xfffff, 0x9a, 0xc);
	set_gdt_entry(2, 0, 0xfffff, 0x92, 0xc);
	set_gdt_entry(3, 0, 0xfffff, 0xfa, 0xc);
	set_gdt_entry(4, 0, 0xfffff, 0xf2, 0xc);
	set_gdt_entry(5, (uint32_t) &tss, sizeof(tss), 0x89, 0x0);
	load_gdt();
	tss[1] = 0x103000;
	tss[2] = 0x10;
}
