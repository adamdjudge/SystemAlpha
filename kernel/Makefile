CC = gcc -m32 -nostdlib -fno-builtin -Wno-write-strings -fno-leading-underscore
AS = as --32
LD = ld -melf_i386

KERNEL = kernel.bin
OBJ = boot.o main.o util.o paging.o console.o idt.o timer.o fdc.o pci.o

all: $(KERNEL)

%.o: %.c
	$(CC) -c -o $@ $<

%.o: %.s
	$(AS) -o $@ $<

$(KERNEL): link.ld $(OBJ)
	$(LD) -T $< -o $@ $(OBJ)

clean:
	rm -rf *.o $(KERNEL)
