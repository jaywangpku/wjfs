#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x7d09bae2, "module_layout" },
	{ 0xa3247381, "per_cpu__current_task" },
	{ 0x27a3b3fa, "kmalloc_caches" },
	{ 0x7700bfd4, "get_sb_nodev" },
	{ 0xad05d389, "save_mount_options" },
	{ 0x64340292, "generic_file_llseek" },
	{ 0x815b5dd4, "match_octal" },
	{ 0xecb174ef, "simple_sync_file" },
	{ 0x67053080, "current_kernel_time" },
	{ 0x3a7805f4, "generic_delete_inode" },
	{ 0x82ac826f, "generic_file_aio_read" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0xc8d6b9dd, "dput" },
	{ 0x44e9a829, "match_token" },
	{ 0x85df9b6c, "strsep" },
	{ 0x9a29b5ce, "page_symlink_inode_operations" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0x7b70f4d7, "generic_file_aio_write" },
	{ 0x70370501, "kbind" },
	{ 0xe2d5255a, "strcmp" },
	{ 0x10cde6fd, "inet_ntoa" },
	{ 0xf3dc79e4, "kill_litter_super" },
	{ 0xbda1a892, "krecv" },
	{ 0xb72397d5, "printk" },
	{ 0xb9295535, "d_rehash" },
	{ 0xab125ed4, "d_alloc_root" },
	{ 0xb4390f9a, "mcount" },
	{ 0xc3811cca, "kconnect" },
	{ 0xc3057517, "ksend" },
	{ 0x84856d65, "inet_addr" },
	{ 0xab9534bb, "simple_getattr" },
	{ 0x9dd454a7, "kmem_cache_alloc" },
	{ 0x5617d2fc, "kaccept" },
	{ 0x4e9b97bd, "simple_unlink" },
	{ 0x40284a8c, "simple_dir_operations" },
	{ 0xbd3cbebf, "generic_file_mmap" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x2008c224, "do_sync_read" },
	{ 0x53b7e44a, "kclose" },
	{ 0xc963c3d5, "wake_up_process" },
	{ 0xee633db3, "generic_show_options" },
	{ 0x225a3e9, "simple_empty" },
	{ 0x98bf2cda, "register_filesystem" },
	{ 0xb7639538, "iput" },
	{ 0x37a0cba, "kfree" },
	{ 0x900b5c5d, "kthread_create" },
	{ 0xa9286fd4, "do_sync_write" },
	{ 0x85a83608, "generic_file_splice_write" },
	{ 0x1c03f068, "simple_statfs" },
	{ 0x5b3fc324, "unregister_filesystem" },
	{ 0xc8297e40, "init_special_inode" },
	{ 0x3b967795, "new_inode" },
	{ 0x14f8c354, "generic_file_splice_read" },
	{ 0x22aba182, "ksocket" },
	{ 0xfd075fb8, "klisten" },
	{ 0x43488a1a, "d_instantiate" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=ksocket";


MODULE_INFO(srcversion, "CCBECEE25CB979840560E84");
