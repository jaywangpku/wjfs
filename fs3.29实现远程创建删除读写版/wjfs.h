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

#define client_port 5007
#define server_port 6000
#define client_ip "192.168.124.133"
#define server_ip "192.168.124.134"

#define test_port 8071
#define test_ip "192.168.124.134"

#define offsetof(TYPE, MEMBER)  ((size_t)(&(((TYPE *)0)->MEMBER)))

#define container_of(ptr, type, member) ({	\
	const typeof(((type *)0)->member) *__mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member));	})

#define RAMFS_DEFAULT_MODE	0755

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
struct reg_ino_mapp_table reg_ino_mapp_table[1024];

//目录节点号映射访问表
struct dir_ino_mapp_table {
	char name[16];
	int local_ino;
	int rdma_ino;
	char ip[16];
};

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

//远程操作文件函数
static int wjfs_mknod_rdma(struct inode *dir, struct dentry *dentry, int mode, dev_t dev);
static int wjfs_rmdir_rdma(struct inode *dir, struct dentry *dentry);
static int wjfs_unlink_rdma(struct inode *dir, struct dentry *dentry);

//普通文件file读操作函数
ssize_t wjfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos);
ssize_t wjfs_file_aio_read(struct kiocb *iocb, const struct iovec *iov, unsigned long nr_segs, loff_t pos);
static void do_generic_file_read(struct file *filp, loff_t *ppos, read_descriptor_t *desc, read_actor_t actor);
int wjfs_file_read_actor(read_descriptor_t *desc, struct page *page, unsigned long offset, unsigned long size);

//普通文件file写操作函数
ssize_t wjfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos);
ssize_t wjfs_file_aio_write(struct kiocb *iocb, const struct iovec *iov, unsigned long nr_segs, loff_t pos);
ssize_t __wjfs_file_aio_write(struct kiocb *iocb, const struct iovec *iov, unsigned long nr_segs, loff_t *ppos);
ssize_t wjfs_file_buffered_write(struct kiocb *iocb, const struct iovec *iov, unsigned long nr_segs, loff_t pos, 
	loff_t *ppos, size_t count, ssize_t written);
static ssize_t wjfs_perform_write(struct file *file, struct iov_iter *i, loff_t pos);
size_t wjfs_iov_iter_copy_from_user_atomic(struct page *page, struct iov_iter *i, unsigned long offset, size_t bytes);
static size_t __wjfs_iovec_copy_from_user_inatomic(char *vaddr, const struct iovec *iov, size_t base, size_t bytes);

//用于通信的函数
static int wjfs_client_create(void *arg, char *ip, int port ,int ino, char *name,
								int ino_parent, char *name_parent, int mode);
static int wjfs_client_remove(void *arg, char *ip, int port ,int ino, char *name,
								int ino_parent, char *name_parent, int mode);
static int wjfs_client_write(void *arg, char *ip, int port ,int ino, char *name,
								int ino_parent, char *name_parent, char *message);
static int wjfs_client_read(void *arg, char *ip, int port ,int ino, char *name,
								int ino_parent, char *name_parent, char *message);
static int wjfs_server(void *arg);

//远端操作函数集
int rdma_create(char *buf, char *ip);
int rdma_remove(char *buf);
int rdma_write(char *buf);
int rdma_read(char *buf);

//获取dentry代表的文件的信息
int get_file_message(struct dentry *dentry, char *message);

//将buf中的内容写入dentry代表的文件中
int put_file_message(struct dentry *dentry, char *message);

//辅助的功能性函数
int string2int(char *buf);
//判断一个文件的所属权是不是本地的
int is_my_file(struct dentry *dentry);

//设计成递归查找所有的dentry  根据名称查找，用于创建文件
struct dentry *find_from_all_dentry(struct dentry *dentry, char *name_parent);
//根据ino号查找文件dentry
struct dentry *find_dentry_by_ino(struct dentry *dentry, int ino);

//获取inode的字节数
static loff_t wjfs_i_size_read(const struct inode *inode);
static inline unsigned wjfs_read_seqcount_begin(const seqcount_t *s);
static inline int wjfs_read_seqcount_retry(const seqcount_t *s, unsigned start);

//在系统内部自动创建文件的函数
void build_file(void);
static int wjfs_create_by_name(const char *name, mode_t mode, struct dentry *parent, struct dentry **dentry);
struct dentry *wjfs_create_dir(const char *name, struct dentry *parent);
struct dentry *wjfs_create_file(const char *name, mode_t mode, struct dentry *parent, void *data, struct file_operations *fops);

//在系统内部创建文件之后，卸载文件系统出现问题，进行修改
void wjfs_kill_litter_super(struct super_block *sb);
void wjfs_kill_anon_super(struct super_block *sb);
void wjfs_d_genocide(struct dentry *root);
void wjfs_shutdown_super(struct super_block *sb);
void wjfs_shrink_dcache_for_umount(struct super_block *sb);
static void wjfs_shrink_dcache_for_umount_subtree(struct dentry *dentry);

//用于测试的相关定义
void get_dentry_name(void);

void mynew_dentry(void);