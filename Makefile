CC = gcc -m32 -nostdlib -fno-builtin -Wno-write-strings -fno-leading-underscore -Iinclude
AS = as --32
LD = ld -melf_i386

OBJ = $(shell ls kernel/*.c kernel/*.s drivers/*.c | sed "s/\../\.o/g" | grep -v fdc | grep -v pci)

all: disk

%.o: %.c
	$(CC) -c -o $@ $<

%.o: %.s
	$(AS) -o $@ $<

kernel: $(OBJ)
	$(LD) -T link.ld -o kernel.bin $(OBJ)

disk: kernel
	./mkbootdisk.sh

run: disk
	qemu-system-i386 -fda sysalpha.img

run-debug: disk
	qemu-system-i386 -fda sysalpha.img -d int,cpu_reset

clean:
	rm -f kernel.bin sysalpha.img kernel/*.o drivers/*.o
