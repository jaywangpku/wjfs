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
	{ 0x27a3b3fa, "kmalloc_caches" },
	{ 0x5ced7b62, "sock_setsockopt" },
	{ 0xbdf2287a, "sock_release" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0x354470e0, "sock_recvmsg" },
	{ 0x3c2c5af5, "sprintf" },
	{ 0xb72397d5, "printk" },
	{ 0x42224298, "sscanf" },
	{ 0x3bbb0d4c, "sock_sendmsg" },
	{ 0xb4390f9a, "mcount" },
	{ 0x9dd454a7, "kmem_cache_alloc" },
	{ 0x63332ec, "sock_create" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "763247421284E9A56C49921");
