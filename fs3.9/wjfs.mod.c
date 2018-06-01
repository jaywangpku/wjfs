#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
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
	{ 0xb669ed0, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x757925ec, __VMLINUX_SYMBOL_STR(generic_show_options) },
	{ 0x85ff88dc, __VMLINUX_SYMBOL_STR(simple_statfs) },
	{ 0xb0f93efd, __VMLINUX_SYMBOL_STR(generic_delete_inode) },
	{ 0x486918ce, __VMLINUX_SYMBOL_STR(simple_getattr) },
	{ 0x247d27fe, __VMLINUX_SYMBOL_STR(simple_setattr) },
	{ 0x121a5af1, __VMLINUX_SYMBOL_STR(generic_file_splice_read) },
	{ 0xf0341c97, __VMLINUX_SYMBOL_STR(iter_file_splice_write) },
	{ 0x29762f93, __VMLINUX_SYMBOL_STR(noop_fsync) },
	{ 0xf70989ef, __VMLINUX_SYMBOL_STR(generic_file_mmap) },
	{ 0x2b418420, __VMLINUX_SYMBOL_STR(generic_file_write_iter) },
	{ 0x88bf1661, __VMLINUX_SYMBOL_STR(generic_file_llseek) },
	{ 0x1937d8f4, __VMLINUX_SYMBOL_STR(simple_write_end) },
	{ 0x7820581c, __VMLINUX_SYMBOL_STR(simple_write_begin) },
	{ 0x61d6d29a, __VMLINUX_SYMBOL_STR(simple_readpage) },
	{ 0xe08f9cb7, __VMLINUX_SYMBOL_STR(unregister_filesystem) },
	{ 0x8815e2cc, __VMLINUX_SYMBOL_STR(register_filesystem) },
	{ 0x4cfc3174, __VMLINUX_SYMBOL_STR(d_make_root) },
	{ 0x815b5dd4, __VMLINUX_SYMBOL_STR(match_octal) },
	{ 0x44e9a829, __VMLINUX_SYMBOL_STR(match_token) },
	{ 0x85df9b6c, __VMLINUX_SYMBOL_STR(strsep) },
	{ 0xe4bd73f1, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x5069bf8c, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x22de5ca8, __VMLINUX_SYMBOL_STR(save_mount_options) },
	{ 0xc8d86a8f, __VMLINUX_SYMBOL_STR(page_symlink_inode_operations) },
	{ 0x153f55f1, __VMLINUX_SYMBOL_STR(simple_dir_operations) },
	{ 0xa9a196f5, __VMLINUX_SYMBOL_STR(init_special_inode) },
	{ 0x4881cb49, __VMLINUX_SYMBOL_STR(inode_init_owner) },
	{ 0xe953b21f, __VMLINUX_SYMBOL_STR(get_next_ino) },
	{ 0x19149bb1, __VMLINUX_SYMBOL_STR(new_inode) },
	{ 0xcb40f365, __VMLINUX_SYMBOL_STR(mark_page_accessed) },
	{ 0x5eb34e2f, __VMLINUX_SYMBOL_STR(add_to_page_cache_lru) },
	{ 0x2fe1269c, __VMLINUX_SYMBOL_STR(__alloc_pages_nodemask) },
	{ 0x4c2fdd72, __VMLINUX_SYMBOL_STR(contig_page_data) },
	{ 0x99be4196, __VMLINUX_SYMBOL_STR(page_cache_sync_readahead) },
	{ 0xb999a92d, __VMLINUX_SYMBOL_STR(copy_page_to_iter) },
	{ 0x8408c732, __VMLINUX_SYMBOL_STR(touch_atime) },
	{ 0x76d80318, __VMLINUX_SYMBOL_STR(iov_iter_advance) },
	{ 0xa1d534b3, __VMLINUX_SYMBOL_STR(filemap_write_and_wait_range) },
	{ 0xcf475f1c, __VMLINUX_SYMBOL_STR(page_cache_async_readahead) },
	{ 0xa3400c98, __VMLINUX_SYMBOL_STR(__lock_page_killable) },
	{ 0x828456b7, __VMLINUX_SYMBOL_STR(put_page) },
	{ 0xa0e76f71, __VMLINUX_SYMBOL_STR(unlock_page) },
	{ 0x725c7388, __VMLINUX_SYMBOL_STR(pagecache_get_page) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0x52784c01, __VMLINUX_SYMBOL_STR(d_set_d_op) },
	{ 0xeea33edb, __VMLINUX_SYMBOL_STR(simple_dentry_operations) },
	{ 0xe0e9f81a, __VMLINUX_SYMBOL_STR(d_rehash) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xbc9431e4, __VMLINUX_SYMBOL_STR(mount_nodev) },
	{ 0xfb015f48, __VMLINUX_SYMBOL_STR(kill_litter_super) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xff196a21, __VMLINUX_SYMBOL_STR(d_instantiate) },
	{ 0xce9e41e9, __VMLINUX_SYMBOL_STR(lockref_get) },
	{ 0xe1beb597, __VMLINUX_SYMBOL_STR(ihold) },
	{ 0x4aac3fbe, __VMLINUX_SYMBOL_STR(inc_nlink) },
	{ 0xd0586751, __VMLINUX_SYMBOL_STR(dput) },
	{ 0x34184afe, __VMLINUX_SYMBOL_STR(current_kernel_time) },
	{ 0x720b9798, __VMLINUX_SYMBOL_STR(simple_unlink) },
	{ 0x6242dcbc, __VMLINUX_SYMBOL_STR(drop_nlink) },
	{ 0xf4268e17, __VMLINUX_SYMBOL_STR(simple_empty) },
	{ 0x50eedeb8, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x8dc6cc1d, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "F3EE0DE2C65FAD78BE45EFA");
