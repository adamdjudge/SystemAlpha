all: disk

kernel.bin:
	cd kernel && make

disk: kernel.bin
	./mkbootdisk.sh

run: disk
	qemu-system-i386 -fda sysalpha.img

clean:
	cd kernel && make clean
	rm -f sysalpha.img
