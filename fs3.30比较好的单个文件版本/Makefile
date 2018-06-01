obj-m += wjfs.o

KBUILD_EXTRA_SYMBOLS=/home/w1/ksocket-0.0.2/src/Module.symvers
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
	mount -t wjfs wjfs /tx

umnt:
	umount /tx

down:
	umount /tx
	rmmod wjfs.ko
	make

up:
	insmod wjfs.ko
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