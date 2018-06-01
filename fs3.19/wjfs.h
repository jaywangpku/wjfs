#include <linux/module.h>     //module
#include <linux/fs.h>         //VFS
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/parser.h>
#include <linux/pagemap.h>
#include <linux/mount.h>
#include <linux/dcache.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/namei.h>
#include <linux/magic.h>      // RAMFS_MAGIC  ??				
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <asm/uaccess.h>
#include <linux/radix-tree.h>     // 使用到基树
#include <linux/uio.h>
#include <linux/swap.h>
#include <net/ksocket.h>
#include <linux/socket.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <net/sock.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <linux/in.h>
#include <linux/kthread.h>
#include <linux/err.h>

#define offsetof(TYPE, MEMBER)  ((size_t)(&(((TYPE *)0)->MEMBER)))

#define container_of(ptr, type, member) ({	\
	const typeof(((type *)0)->member) *__mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member));	})

#define RAMFS_DEFAULT_MODE	0755

//超级块的相关操作函数对象
static const struct super_operations wjfs_ops;

//文件系统类型的相关对象
static struct file_system_type wjfs_fs_type;

//目录inode操作函数对象
static const struct inode_operations wjfs_dir_inode_operations;

//目录file操作函数对象
const struct file_operations wjfs_dir_file_operations;

//普通文件inode操作函数对象 mmu部分
const struct inode_operations wjfs_file_inode_operations;

//普通文件file操作函数对象 mmu部分
const struct file_operations wjfs_file_operations;

//dentry 操作函数对象  ramfs simple_dentry_operations 只是删除 没有意义
static const struct dentry_operations wjfs_dentry_operations;

//后备地址空间操作（内存文件系统不用管）
static struct backing_dev_info wjfs_backing_dev_info;
static const struct address_space_operations wjfs_aops ;

//获取inode函数
struct inode *wjfs_get_inode(struct super_block *sb, int mode, dev_t dev);

//目录inode操作函数  wjfs_dir_inode_operations
static int wjfs_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t dev);
static int wjfs_mkdir(struct inode *dir, struct dentry *dentry, int mode);
static int wjfs_create(struct inode *dir, struct dentry *dentry, int mode, struct nameidata *nd);
static int wjfs_rename(struct inode *old_dir, struct dentry *old_dentry,
							struct inode *new_dir, struct dentry *new_dentry);
static struct dentry *wjfs_lookup(struct inode *dir, struct dentry *dentry, struct nameidata *nd);
static int wjfs_rmdir(struct inode *dir, struct dentry *dentry);
static int wjfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname);
static int wjfs_link(struct dentry *old_dentry, struct inode *dir, struct dentry *dentry);
static int wjfs_unlink(struct inode *dir, struct dentry *dentry);

//普通文件file操作函数
static ssize_t wjfs_file_read_iter(struct kiocb *iocb, struct iov_iter *iter);

//用于通信的函数
static int wjfs_client(void *arg);
static int wjfs_server(void *arg);

//用于测试的相关定义
void get_dentry_name(void);

void mynew_dentry(void);

void build_file(void);
static int wjfs_create_by_name(const char *name, mode_t mode, struct dentry *parent, struct dentry **dentry);
struct dentry *wjfs_create_dir(const char *name, struct dentry *parent);
struct dentry *wjfs_create_file(const char *name, mode_t mode, struct dentry *parent, void *data, struct file_operations *fops);
