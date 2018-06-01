#ifndef _WJFS_H
#define _WJFS_H

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
#include <linux/hash.h>
#include <linux/bootmem.h>
#include <linux/seqlock.h>
#include <linux/cache.h>

#define test_port 8078
#define test_ip "192.168.124.134"

#define offsetof(TYPE, MEMBER)  ((size_t)(&(((TYPE *)0)->MEMBER)))

#define container_of(ptr, type, member) ({	\
	const typeof(((type *)0)->member) *__mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member));	})

#define RAMFS_DEFAULT_MODE	0755

//定义全局变量
extern struct super_block *rroot_sb;
extern struct dentry *rroot_dentry;
extern struct inode *rroot_inode;
extern struct dentry *a_dentry;

//自定义结构体
//文件节点号映射访问表
struct reg_ino_mapp_table {
	char name[16];          //名称
	int local_ino;          //本地节点号
	int rdma_ino;			//远端节点号
	int local_parent_ino;   //本地父节点号
	int rdma_parent_ino;	//远端父节点号
	char ip[16];            //远端访问的ip地址
	int used;               //判断该表项是否已经被使用
};
extern struct reg_ino_mapp_table reg_ino_mapp_table[1024];

//超级块的相关操作函数对象
static struct super_operations wjfs_ops;

//文件系统类型的相关对象
static struct file_system_type wjfs_fs_type;

//目录inode操作函数对象
static struct inode_operations wjfs_dir_inode_operations;

//普通文件inode操作函数对象 mmu部分
static struct inode_operations wjfs_file_inode_operations;

//普通文件file操作函数对象 mmu部分
static struct file_operations wjfs_file_operations;

//后备地址空间操作（内存文件系统不用管）
static struct backing_dev_info wjfs_backing_dev_info;
static struct address_space_operations wjfs_aops ;

//获取inode函数
extern struct inode *wjfs_get_inode(struct super_block *sb, int mode, dev_t dev);

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

extern void wait_on_retry_sync_kiocb(struct kiocb *iocb);

//用于通信的函数
extern int wjfs_server(void *arg);

//获取dentry代表的文件的信息
extern int get_file_message(struct dentry *dentry, char *message);

//将buf中的内容写入dentry代表的文件中
extern int put_file_message(struct dentry *dentry, char *message);

//辅助的功能性函数
extern int string2int(char *buf);
//判断一个文件的所属权是不是本地的
extern int is_my_file(struct dentry *dentry);

//设计成递归查找所有的dentry  根据名称查找，用于创建文件
extern struct dentry *find_from_all_dentry(struct dentry *dentry, char *name_parent);
//根据ino号查找文件dentry
extern struct dentry *find_dentry_by_ino(struct dentry *dentry, int ino);

//在系统内部创建文件之后，卸载文件系统出现问题，进行修改
extern void wjfs_kill_litter_super(struct super_block *sb);
extern void wjfs_d_genocide(struct dentry *root);

#endif