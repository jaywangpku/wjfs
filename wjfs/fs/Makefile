obj-m += myfs.o

myfs-objs := wjfs.o wjfs_create.o wjfs_remove.o wjfs_read.o wjfs_write.o

KBUILD_EXTRA_SYMBOLS=/home/w1/ksocket-0.0.2/src/Module.symvers
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

in:
	insmod myfs.ko

rm:
	rmmod myfs.ko

fs:
	cat /proc/filesystems

see:
	dmesg -c

mnt:
	mount -t wjfs wjfs /tx

umnt:
	umount /tx

down:
	umount /tx
	rmmod myfs.ko
	make

up:
	insmod myfs.ko
	mount -t wjfs wjfs /tx
	dmesg -c

build:
	touch /tx/a
	chmod 777 /tx/a
	touch /tx/b
	mkdir /tx/c
	mkdir /tx/d
	touch /tx/c/e

ubuild:
	rm -rf /tx/a /tx/b /tx/c /tx/d

haha:
	make down
	make up
	make build
	make see