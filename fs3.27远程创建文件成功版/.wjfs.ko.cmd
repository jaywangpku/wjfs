cmd_/fs/wjfs.ko := ld -r -m elf_i386 -T /usr/src/linux-headers-2.6.32-21-generic/scripts/module-common.lds --build-id -o /fs/wjfs.ko /fs/wjfs.o /fs/wjfs.mod.o
