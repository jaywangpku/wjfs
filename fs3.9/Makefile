obj-m += wjfs.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

in:
	insmod wjfs.ko

rm:
	rmmod wjfs.ko

fs:
	cat /proc/filesystems

see:
	dmesg -c

mnt:
	mount -t wjfs wjfs /mnt/m0

umnt:
	umount /mnt/m0

down:
	umount /mnt/m0
	rmmod wjfs.ko
	make

up:
	insmod wjfs.ko
	mount -t wjfs wjfs /mnt/m0
	dmesg -c
	make build
	dmesg -c

build:
	touch /mnt/m0/a
	touch /mnt/m0/b
	mkdir /mnt/m0/c
	mkdir /mnt/m0/d
	touch /mnt/m0/c/e

tree:
	tree /mnt/m0

ubuild:
	rm -rf /mnt/m0/a /mnt/m0/b /mnt/m0/c /mnt/m0/d

haha:
	make down
	make up
	make build
	make tree
	make see