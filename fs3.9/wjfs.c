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
#include "wjfs.h"

#define offsetof(TYPE, MEMBER)  ((size_t)(&(((TYPE *)0)->MEMBER)))

#define container_of(ptr, type, member) ({	\
	const typeof(((type *)0)->member) *__mptr = (ptr);	\
	(type *)((char *)__mptr - offsetof(type, member));	})

#define RAMFS_DEFAULT_MODE	0755

struct super_block *rroot_sb;       //指向super_block的指针
struct dentry *rroot_dentry;        //
struct inode *rroot_inode;
struct dentry *a_dentry;

//后备地址空间操作（内存文件系统不用管）
static const struct address_space_operations wjfs_aops = {
	.readpage                  = simple_readpage,
	.write_begin               = simple_write_begin,
	.write_end                 = simple_write_end,
	//.set_page_dirty            = __set_page_dirty_no_writeback,
};

struct inode *wjfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode, dev_t dev)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct inode *inode = new_inode(sb);     // ??

	if(inode)
	{
		inode->i_ino = get_next_ino();   // ??
		
		printk(KERN_EMERG"inode->i_ino : %d",inode->i_ino);

		//Init uid,gid,mode for new inode according to posix standards
		inode_init_owner(inode, dir, mode);
		inode->i_mapping->a_ops = &wjfs_aops;

		//Only to be used before the mapping is activated.  ??
		mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
		mapping_set_unevictable(inode->i_mapping);

		inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;

		switch(mode & S_IFMT)
		{
			default:
				init_special_inode(inode, mode, dev);
				break;
			case S_IFREG:
				inode->i_op = &wjfs_file_inode_operations;
				inode->i_fop = &wjfs_file_operations;
				break;
			case S_IFDIR:
				inode->i_op = &wjfs_dir_inode_operations;
				inode->i_fop = &simple_dir_operations;
				
				/* directory inodes start off with i_nlink == 2 (for "." entry) */
				// directly increment an inode's link count
				inc_nlink(inode);
				break;
			case S_IFLNK:
				inode->i_op = &page_symlink_inode_operations;
				break;
		}
	}
	return inode;
}

/********************************************************************************************************************/
/********************************************************************************************************************/

//what dir used for ??
//create a new special dev file, file in dir and it's dentry in dentry
static int wjfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	// if(dir)
	// 	printk("wjfs_mknod dir: %ld\n",dir->i_ino);
	// if(dentry)
	// 	printk("wjfs_mknod dentry: %s\n",dentry->d_name.name);
	struct inode *inode = wjfs_get_inode(dir->i_sb, dir, mode, dev);
	int err = -ENOSPC;

	if(inode)
	{
		//fill in inode information for a dentry
		d_instantiate(dentry, inode);
		//dput表示用户释放文件目录项dentry并将其放入缓冲池中，dget表示用户对dentry进行引用并增加它的引用计数。 
		dget(dentry);

		err = 0;
		dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	}
	return err;
}

static int wjfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	int retval = wjfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if(!retval)
		inc_nlink(dir);
	return retval;
}

static int wjfs_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	return wjfs_mknod(dir, dentry, mode | S_IFREG, 0);
}

static int wjfs_rename(struct inode *old_dir, struct dentry *old_dentry,
							struct inode *new_dir, struct dentry *new_dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	printk(KERN_EMERG"old_dentry->d_name.name:  %s\n",old_dentry->d_name.name);
	printk(KERN_EMERG"new_dentry->d_name.name:  %s\n",new_dentry->d_name.name);
	// printk(KERN_EMERG"parent_dentry_old->d_name.name:  %s\n",old_dentry->d_parent->d_name.name);
	// printk(KERN_EMERG"parent_dentry_new->d_name.name:  %s\n",new_dentry->d_parent->d_name.name);
	
	struct inode *inode = d_inode(old_dentry);           //d_inode 对 dentry->d_inode 的封装
	int they_are_dirs = d_is_dir(old_dentry);

	if (!simple_empty(new_dentry))
		return -ENOTEMPTY;

	if (d_really_is_positive(new_dentry)) {
		simple_unlink(new_dir, new_dentry);
		if (they_are_dirs) {
			drop_nlink(d_inode(new_dentry));
			drop_nlink(old_dir);
		}
	} else if (they_are_dirs) {
		drop_nlink(old_dir);
		inc_nlink(new_dir);
	}

	old_dir->i_ctime = old_dir->i_mtime = new_dir->i_ctime =
		new_dir->i_mtime = inode->i_ctime = CURRENT_TIME;

	return 0;
}

//在特定的目录中寻找索引节点,该索引节点对应dentry中给定的文件名
static struct dentry *wjfs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	// if(dir)
	// 	printk(KERN_EMERG"wjfs_lookup_dir_i_ino:  %ld\n",dir->i_ino);
	// if(dentry)
	// 	printk(KERN_EMERG"dentry->d_name.name:  %s\n",dentry->d_name.name);
	// if(dentry->d_inode)
	// 	printk(KERN_EMERG"dentry->d_inode.i_ino:  %ld\n",dentry->d_inode->i_ino);
	if(strcmp(dentry->d_name.name,"a")==0)
	{
		printk(KERN_EMERG" 获得a_dentry信息 \n");
		a_dentry = dentry;
	}
	
	if (dentry->d_name.len > NAME_MAX)
		return ERR_PTR(-ENAMETOOLONG);
	if (!dentry->d_sb->s_d_op)
		d_set_d_op(dentry, &simple_dentry_operations);
	d_add(dentry, NULL);
	return NULL;
}

//删除dir目录中的dentry目录项代表的文件
static int wjfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	if (!simple_empty(dentry))
		return -ENOTEMPTY;

	drop_nlink(d_inode(dentry));
	simple_unlink(dir, dentry);
	drop_nlink(dir);
	return 0;
}

// 符号链接的名称在dentry中，链接对象是dir目录中的名称为symname		此时作为调试的触发函数
static int wjfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	//test
	void *arg;
	if(strcmp("s",dentry->d_name.name) == 0)
	{
		wjfs_server(arg);
	}
	if(strcmp("c",dentry->d_name.name) == 0)
	{
		wjfs_client(arg);
	}
	printk("error\n");
	
	return 0;
}

//硬链接名称由dentry参数指定，链接对象是dir目录中old_dentry目录项所代表的文件
static int wjfs_link(struct dentry *old_dentry, struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct inode *inode = d_inode(old_dentry);

	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	inc_nlink(inode);
	ihold(inode);
	dget(dentry);
	d_instantiate(dentry, inode);
	return 0;
}

//从目录dir中删除dentry指定的索引节点对象
static int wjfs_unlink(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct inode *inode = d_inode(dentry);

	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	drop_nlink(inode);
	dput(dentry);
	return 0;
}


static const struct inode_operations wjfs_dir_inode_operations = {
	.create                    = wjfs_create,		  //创建普通文件
	.mkdir                     = wjfs_mkdir,		  //创建目录文件
	.mknod                     = wjfs_mknod,		  //创建特殊设备文件
	.rename                    = wjfs_rename,         //修改文件名称 
	.lookup                    = wjfs_lookup,         //可能是路径查找
	.rmdir                     = wjfs_rmdir,
	.symlink                   = wjfs_symlink,        //此时作为调试的触发函数 
	.link                      = wjfs_link,
	.unlink                    = wjfs_unlink,

	//还没有实现但是需要用到的函数
	//.lookup     = simple_lookup,
	//.link		= simple_link,
	//.unlink		= simple_unlink,
	//.rename		= simple_rename,
	//.rmdir		= simple_rmdir,
};

/********************************************************************************************************************/
/********************************************************************************************************************/

//用于超级块的相关操作
//无法识别根目录有可能就在这个地方有问题
static const struct super_operations wjfs_ops = {
	.statfs                 = simple_statfs,
	.drop_inode             = generic_delete_inode,
	.show_options           = generic_show_options,
};

struct wjfs_mount_opts {
	umode_t mode;
};

enum {
	Opt_mode,
	Opt_err
};

static const match_table_t tokens = {
	{Opt_mode, "mode=%o"},
	{Opt_err, NULL}
};

struct wjfs_fs_info {
	struct wjfs_mount_opts mount_opts;
};

//还没有理解的函数
static int wjfs_parse_options(char *data, struct wjfs_mount_opts *opts)
{
	substring_t args[MAX_OPT_ARGS];
	int option;
	int token;
	char *p;

	opts->mode = RAMFS_DEFAULT_MODE;

	while ((p = strsep(&data, ",")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case Opt_mode:
			if (match_octal(&args[0], &option))
				return -EINVAL;
			opts->mode = option & S_IALLUGO;
			break;
		/*
		 * We might like to report bad mount options here;
		 * but traditionally ramfs has ignored all mount options,
		 * and as it is used as a !CONFIG_SHMEM simple substitute
		 * for tmpfs, better continue to ignore other mount options.
		 */
		}
	}
	return 0;
}

int wjfs_fill_super(struct super_block *sb, void *data, int silent)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	
	//data 输出为null
	//printk("data : %s\n",data);
	
	struct wjfs_fs_info *fsi;
	struct inode *root_inode;
	struct dentry *root_dentry;
	int err;

	//If filesystem uses generic_show_options() in wjfs_ops , this function should be called from the fill_super() callback.
	save_mount_options(sb, data);

	// s_fs_info 文件系统特殊信息
	fsi = kzalloc(sizeof(struct wjfs_fs_info),GFP_KERNEL);
	sb->s_fs_info = fsi;
	if(!fsi)
		return -ENOMEM;
	printk("fsi = %d\n",fsi);

	// ??
	err = wjfs_parse_options(data, &fsi->mount_opts);
	if(err)
		return err;

	//一些其他属性的设置
	sb->s_maxbytes                 = MAX_LFS_FILESIZE;        //文件大小的上限
	sb->s_blocksize                = PAGE_SIZE;
	sb->s_blocksize_bits           = PAGE_SHIFT;
	sb->s_magic                    = RAMFS_MAGIC;             // ??
	sb->s_op                       = &wjfs_ops;
	sb->s_time_gran                = 1;                       //时间戳粒度

	root_inode = wjfs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
	//root_inode = wjfs_get_inode(sb, NULL, S_IFDIR, 0);

	sb->s_root = d_make_root(root_inode);                     //s_root 目录挂载点

	//通过全局变量将三个导出来方便使用
	rroot_sb = sb;
	rroot_dentry = sb->s_root;
	rroot_inode = sb->s_root->d_inode;

	// look for the info of root inode and dentry
	struct qstr root_name_dentry;
	root_name_dentry = sb->s_root->d_name;
	
	// 访问根目录节点下的所有子目录项
	// struct list_head *subdirs;
	// struct list_head *plist;
	// struct dentry *sub_dentry;
	// subdirs = &(sb->s_root->d_subdirs);
	
	// list_for_each(plist,subdirs)
	// {
	// 	sub_dentry = list_entry(plist, struct dentry, d_subdirs);
	// 	printk(KERN_EMERG"subdirs_dentry %s",sub_dentry->d_name.name);
	// }
	
	//dentry info
	printk(KERN_EMERG"root_name_dentry %s",root_name_dentry.name);
	printk(KERN_EMERG"root_parent_name_dentry %s",sb->s_root->d_parent->d_name.name);
	//inode info
	printk(KERN_EMERG"root_inode_num %ld",root_inode->i_ino);

	if(!sb->s_root)
		return -ENOMEM;

	return 0;
}

static struct dentry *wjfs_mount(struct file_system_type *fs_type,int flags,const char *dev_name,void *data)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);

	struct dentry *mount_dentry;
	struct qstr name_dentry;
	//主要的作用是调用wjfs_fill_super去填充超级块
	//error = fill_super(s, data, flags & MS_SILENT ? 1 : 0);
	//sget得到已经存在或者新分配的super_block结构
	//struct super_block *s = sget(fs_type, NULL, set_anon_super, flags, NULL);
	mount_dentry = mount_nodev(fs_type, flags, data, wjfs_fill_super);
	
	//查看挂载点的根目录名称
	name_dentry = mount_dentry->d_name;
	if(!mount_dentry)
	{
		printk(KERN_EMERG"mount_dentry \n");
	}
	else
	{
		printk(KERN_EMERG"name_root_dentry %s\n",name_dentry.name);
	}
	//data 指向一个与文件系统相关的结构，可以为NULL
	return mount_dentry;
}

void wjfs_kill_sb(struct super_block *sb)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	/* 卸载一个文件系统时要做的操作 */
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
}

static struct file_system_type wjfs_fs_type = {
	.name           = "wjfs",
	.mount          = wjfs_mount,                       // mount -t wjfs wjfs /mnt 执行此函数
	.kill_sb        = wjfs_kill_sb,
	.fs_flags       = FS_USERNS_MOUNT,
};

static int __init init_wjfs(void)
{
	printk("register_filesystem\n");
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);

	int retval;
	retval = register_filesystem(&wjfs_fs_type);
	return retval;
}

static void __exit exit_wjfs(void)
{
	printk("unregister_filesystem\n");
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	unregister_filesystem(&wjfs_fs_type);
}

module_init(init_wjfs);
module_exit(exit_wjfs);
MODULE_LICENSE("GPL");        //"GPL" 是指明了 这是GNU General Public License的任意版本



/********************************************************************************************************************/
/********************************************************************************************************************/

static unsigned long wjfs_mmu_get_unmapped_area(struct file *file,
		unsigned long addr, unsigned long len, unsigned long pgoff,
		unsigned long flags)
{
	return current->mm->get_unmapped_area(file, addr, len, pgoff, flags);
}

static void shrink_readahead_size_eio(struct file *filp,
					struct file_ra_state *ra)
{
	ra->ra_pages /= 4;
}

/**
 * do_generic_file_read - generic file read routine
 * @filp:	the file to read
 * @ppos:	current file position
 * @iter:	data destination
 * @written:	already copied
 *
 * This is a generic file read routine, and uses the
 * mapping->a_ops->readpage() function for the actual low-level stuff.
 *
 * This is really ugly. But the goto's actually try to clarify some
 * of the logic when it comes to error handling etc.
 */
static ssize_t do_generic_file_read(struct file *filp, loff_t *ppos,
		struct iov_iter *iter, ssize_t written)
{
	struct address_space *mapping = filp->f_mapping;
	struct inode *inode = mapping->host;
	struct file_ra_state *ra = &filp->f_ra;
	pgoff_t index;
	pgoff_t last_index;
	pgoff_t prev_index;
	unsigned long offset;      /* offset into pagecache page */
	unsigned int prev_offset;
	int error = 0;

	index = *ppos >> PAGE_CACHE_SHIFT;
	prev_index = ra->prev_pos >> PAGE_CACHE_SHIFT;
	prev_offset = ra->prev_pos & (PAGE_CACHE_SIZE-1);
	last_index = (*ppos + iter->count + PAGE_CACHE_SIZE-1) >> PAGE_CACHE_SHIFT;
	offset = *ppos & ~PAGE_CACHE_MASK;

	for (;;) {
		struct page *page;
		pgoff_t end_index;
		loff_t isize;
		unsigned long nr, ret;

		cond_resched();
find_page:
		page = find_get_page(mapping, index);
		if (!page) {
			page_cache_sync_readahead(mapping,
					ra, filp,
					index, last_index - index);
			page = find_get_page(mapping, index);
			if (unlikely(page == NULL))
				goto no_cached_page;
		}
		if (PageReadahead(page)) {
			page_cache_async_readahead(mapping,
					ra, filp, page,
					index, last_index - index);
		}
		if (!PageUptodate(page)) {
			if (inode->i_blkbits == PAGE_CACHE_SHIFT ||
					!mapping->a_ops->is_partially_uptodate)
				goto page_not_up_to_date;
			if (!trylock_page(page))
				goto page_not_up_to_date;
			/* Did it get truncated before we got the lock? */
			if (!page->mapping)
				goto page_not_up_to_date_locked;
			if (!mapping->a_ops->is_partially_uptodate(page,
							offset, iter->count))
				goto page_not_up_to_date_locked;
			unlock_page(page);
		}
page_ok:
		/*
		 * i_size must be checked after we know the page is Uptodate.
		 *
		 * Checking i_size after the check allows us to calculate
		 * the correct value for "nr", which means the zero-filled
		 * part of the page is not copied back to userspace (unless
		 * another truncate extends the file - this is desired though).
		 */

		isize = i_size_read(inode);
		end_index = (isize - 1) >> PAGE_CACHE_SHIFT;
		if (unlikely(!isize || index > end_index)) {
			page_cache_release(page);
			goto out;
		}

		/* nr is the maximum number of bytes to copy from this page */
		nr = PAGE_CACHE_SIZE;
		if (index == end_index) {
			nr = ((isize - 1) & ~PAGE_CACHE_MASK) + 1;
			if (nr <= offset) {
				page_cache_release(page);
				goto out;
			}
		}
		nr = nr - offset;

		/* If users can be writing to this page using arbitrary
		 * virtual addresses, take care about potential aliasing
		 * before reading the page on the kernel side.
		 */
		if (mapping_writably_mapped(mapping))
			flush_dcache_page(page);

		/*
		 * When a sequential read accesses a page several times,
		 * only mark it as accessed the first time.
		 */
		if (prev_index != index || offset != prev_offset)
			mark_page_accessed(page);
		prev_index = index;

		/*
		 * Ok, we have the page, and it's up-to-date, so
		 * now we can copy it to user space...
		 */

		ret = copy_page_to_iter(page, offset, nr, iter);
		offset += ret;
		index += offset >> PAGE_CACHE_SHIFT;
		offset &= ~PAGE_CACHE_MASK;
		prev_offset = offset;

		page_cache_release(page);
		written += ret;
		if (!iov_iter_count(iter))
			goto out;
		if (ret < nr) {
			error = -EFAULT;
			goto out;
		}
		continue;

page_not_up_to_date:
		/* Get exclusive access to the page ... */
		error = lock_page_killable(page);
		if (unlikely(error))
			goto readpage_error;

page_not_up_to_date_locked:
		/* Did it get truncated before we got the lock? */
		if (!page->mapping) {
			unlock_page(page);
			page_cache_release(page);
			continue;
		}

		/* Did somebody else fill it already? */
		if (PageUptodate(page)) {
			unlock_page(page);
			goto page_ok;
		}

readpage:
		/*
		 * A previous I/O error may have been due to temporary
		 * failures, eg. multipath errors.
		 * PG_error will be set again if readpage fails.
		 */
		ClearPageError(page);
		/* Start the actual read. The read will unlock the page. */
		error = mapping->a_ops->readpage(filp, page);

		if (unlikely(error)) {
			if (error == AOP_TRUNCATED_PAGE) {
				page_cache_release(page);
				error = 0;
				goto find_page;
			}
			goto readpage_error;
		}

		if (!PageUptodate(page)) {
			error = lock_page_killable(page);
			if (unlikely(error))
				goto readpage_error;
			if (!PageUptodate(page)) {
				if (page->mapping == NULL) {
					/*
					 * invalidate_mapping_pages got it
					 */
					unlock_page(page);
					page_cache_release(page);
					goto find_page;
				}
				unlock_page(page);
				shrink_readahead_size_eio(filp, ra);
				error = -EIO;
				goto readpage_error;
			}
			unlock_page(page);
		}

		goto page_ok;

readpage_error:
		/* UHHUH! A synchronous read error occurred. Report it */
		page_cache_release(page);
		goto out;

no_cached_page:
		/*
		 * Ok, it wasn't cached, so we need to create a new
		 * page..
		 */
		page = page_cache_alloc_cold(mapping);
		if (!page) {
			error = -ENOMEM;
			goto out;
		}
		error = add_to_page_cache_lru(page, mapping, index,
					GFP_KERNEL & mapping_gfp_mask(mapping));
		if (error) {
			page_cache_release(page);
			if (error == -EEXIST) {
				error = 0;
				goto find_page;
			}
			goto out;
		}
		goto readpage;
	}

out:
	ra->prev_pos = prev_index;
	ra->prev_pos <<= PAGE_CACHE_SHIFT;
	ra->prev_pos |= prev_offset;

	*ppos = ((loff_t)index << PAGE_CACHE_SHIFT) + offset;
	file_accessed(filp);
	return written ? written : error;
}

static ssize_t wjfs_file_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);

	struct file *file = iocb->ki_filp;
	ssize_t retval = 0;
	loff_t *ppos = &iocb->ki_pos;
	loff_t pos = *ppos;

	if (iocb->ki_flags & IOCB_DIRECT) {
		struct address_space *mapping = file->f_mapping;
		struct inode *inode = mapping->host;
		size_t count = iov_iter_count(iter);
		loff_t size;

		if (!count)
			goto out; /* skip atime */
		size = i_size_read(inode);
		retval = filemap_write_and_wait_range(mapping, pos,
					pos + count - 1);
		if (!retval) {
			struct iov_iter data = *iter;
			retval = mapping->a_ops->direct_IO(iocb, &data, pos);
		}

		if (retval > 0) {
			*ppos = pos + retval;
			iov_iter_advance(iter, retval);
		}

		/*
		 * Btrfs can have a short DIO read if we encounter
		 * compressed extents, so if there was an error, or if
		 * we've already read everything we wanted to, or if
		 * there was a short read because we hit EOF, go ahead
		 * and return.  Otherwise fallthrough to buffered io for
		 * the rest of the read.  Buffered reads will not work for
		 * DAX files, so don't bother trying.
		 */
		if (retval < 0 || !iov_iter_count(iter) || *ppos >= size ||
		    IS_DAX(inode)) {
			file_accessed(file);
			goto out;
		}
	}

	retval = do_generic_file_read(file, ppos, iter, retval);
out:
	return retval;
}

const struct file_operations wjfs_file_operations = {
	.read_iter	= wjfs_file_read_iter,
	.write_iter	= generic_file_write_iter,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		= generic_file_llseek,
	.get_unmapped_area	= wjfs_mmu_get_unmapped_area,
};

const struct inode_operations wjfs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};

static int wjfs_client(void *arg)
{
	ksocket_t sockfd_cli;
	struct sockaddr_in addr_srv;
	
	char buf[1024];
	char *tmp;
	int addr_len;
	int len;

#ifdef KSOCKET_ADDR_SAFE
	mm_segment_t old_fs;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
#endif

	memset(&addr_srv, 0, sizeof(addr_srv));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(9669);
	addr_srv.sin_addr.s_addr = inet_addr("192.168.124.128");
	addr_len = sizeof(struct sockaddr_in);

	sockfd_cli = ksocket(AF_INET, SOCK_STREAM, 0);
	printk("sockfd_cli = 0x%p\n", sockfd_cli);
	if(sockfd_cli == NULL)
	{
		printk("ksocket failed\n");
		return -1;
	}
	printk("ksocket ok\n");

	if(kconnect(sockfd_cli, (struct sockaddr *)&addr_srv, addr_len) < 0)
	{
		printk("kconnect failed\n");
		return -1;
	}
	printk("kconnect ok\n");

	//用于输出服务端的ip和端口号信息
	tmp = inet_ntoa(&addr_srv.sin_addr);
	printk("client connected to : %s %d\n", tmp, ntohs(addr_srv.sin_port));
	kfree(tmp);

	len = sprintf(buf, "%s", "Hello, I am client\0");
	ksend(sockfd_cli, buf, len, 0);
	// memset(buf, '\0', sizeof(buf));
	// strcpy(buf, "Hello, I am client");
	// len = sizeof(buf);
	// ksend(sockfd_cli, buf, len, 0);

	krecv(sockfd_cli, buf, 1024, 0);
	printk("client got message : %s\n", buf);

	kclose(sockfd_cli);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}

static int wjfs_server(void *arg)
{
	ksocket_t sockfd_srv, sockfd_cli;
	struct sockaddr_in addr_srv;
	struct sockaddr_in addr_cli;
	char buf[1024];
	char *tmp;
	int addr_len;
	int len;

/*for safe*/
#ifdef KSOCKET_ADDR_SAFE
		mm_segment_t old_fs;
		old_fs = get_fs();
		set_fs(KERNEL_DS);
#endif

	sockfd_srv = sockfd_cli = NULL;
	memset(&addr_srv, 0, sizeof(addr_srv));
	memset(&addr_cli, 0, sizeof(addr_cli));
	addr_srv.sin_family = AF_INET;
	addr_srv.sin_port = htons(9669);
	addr_srv.sin_addr.s_addr = INADDR_ANY;
	addr_len = sizeof(struct sockaddr_in);

	sockfd_srv = ksocket(AF_INET, SOCK_STREAM, 0);
	if(sockfd_srv == NULL)
	{
		printk("ksocket failed\n");
		return -1;
	}
	printk("ksocket ok\n");

	if(kbind(sockfd_srv, (struct sockaddr *)&addr_srv, addr_len) < 0)
	{
		printk("kbind failed\n");
		return -1;
	}
	printk("kbind ok\n");

	if(klisten(sockfd_srv, 10) < 0)
	{
		printk("klisten failed\n");
		return -1;
	}
	printk("klisten ok\n");

	printk("before kaccept\n");
	sockfd_cli = kaccept(sockfd_srv, (struct sockaddr *)&addr_cli, &addr_len);
	printk("after kaccept\n");
	if(sockfd_cli == NULL)
	{
		printk("kaccept failed\n");
		return -1;
	}
	printk("kaccept ok\n");
	printk("sockfd_cli = 0x%p\n", sockfd_cli);

	//用于输出客户端的ip和端口号信息
	tmp = inet_ntoa(&addr_cli.sin_addr);
	printk("got connected from : %s %d\n", tmp, ntohs(addr_cli.sin_port));
	kfree(tmp);
	
	memset(buf, 0, sizeof(buf));
	printk("before krecv\n");
	len = krecv(sockfd_cli, buf, sizeof(buf), 0);
	printk("after krecv\n");
	if(len > 0)
	{
		printk("server got message : %s\n", buf);
		// if(memcmp(buf, "quit", 4) == 0)
		// 	break;

		len = sprintf(buf, "%s", "Hello, welcome to ksocket server\0");
		printk("before ksend\n");
		ksend(sockfd_cli, buf, len, 0);
		printk("after ksend\n");
	}

	kclose(sockfd_cli);
	kclose(sockfd_srv);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}




