# 
# ksocket project
# BSD-style socket APIs for kernel 2.6 developers
# 
# @2007-2008, China
# @song.xian-guang@hotmail.com (MSN Accounts)
# 
# This code is licenced under the GPL
# Feel free to contact me if any questions
# 

.PHONY: default clean

obj-m := ksocket_tcp_srv_demo.o ksocket_tcp_cli_demo.o
#ksocket_tcp_srv-objs := ../ksocket.o
KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

EXTRA_LDFLAGS := -I../../src
KBUILD_EXTRA_SYMBOLS=/home/w1/ksocket-0.0.2/src/Module.symvers

ifeq ($(ADDRSAFE),true)
	EXTRA_CFLAGS += -DKSOCKET_ADDR_SAFE
endif

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.ko *.o *.mod.* .H* .tm* .*cmd Module.symvers

