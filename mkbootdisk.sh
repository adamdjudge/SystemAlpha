#!/bin/bash

BOOTDISK=sysalpha.img
KERNEL=kernel.bin

mkdir -p mnt
cp boot/bootgrub.img $BOOTDISK
sudo mount $BOOTDISK ./mnt
sudo cp $KERNEL ./mnt/boot
sudo cp boot/menu.lst ./mnt/boot/grub
sudo umount ./mnt
rm -fr ./mnt
