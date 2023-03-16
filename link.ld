ENTRY(start)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)

SECTIONS
{
	. = 0x100000;

	.text :
	{
		*(.multiboot)
		*(.text)
	}

	.bss ALIGN (4K) :
	{
		kernel_code_end = .;
		*(.bss)
	}

	.data ALIGN (4K) :
	{
		*(.data)
		*(.rodata)
		kernel_end = .;
	}

	/DISCARD/ :
	{
		*(.fini_array)
		*(.comment)
	}
}
