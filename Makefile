all: disk

kernel.bin:
	cd kernel && make

disk: kernel.bin
	./mkbootdisk.sh

run: disk
	qemu-system-i386 -fda sysalpha.img

run-debug: disk
	qemu-system-i386 -fda sysalpha.img -d int,cpu_reset

clean:
	cd kernel && make clean
	rm -f sysalpha.img
