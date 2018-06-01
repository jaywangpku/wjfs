#include "wjfs.h"

struct super_block *rroot_sb;
struct dentry *rroot_dentry;
struct inode *rroot_inode;
struct dentry *a_dentry;

static struct backing_dev_info wjfs_backing_dev_info = {
	.name		= "wjfs",
	.ra_pages	= 0,	/* No readahead */
	.capabilities	= BDI_CAP_NO_ACCT_AND_WRITEBACK |
			  BDI_CAP_MAP_DIRECT | BDI_CAP_MAP_COPY |
			  BDI_CAP_READ_MAP | BDI_CAP_WRITE_MAP | BDI_CAP_EXEC_MAP,
};

int __wjfs_set_page_dirty_no_writeback(struct page *page)
{
	if (!PageDirty(page))
		SetPageDirty(page);
	return 0;
}

static const struct address_space_operations wjfs_aops = {
	.readpage		= simple_readpage,
	.write_begin		= simple_write_begin,
	.write_end		= simple_write_end,
	.set_page_dirty		= __wjfs_set_page_dirty_no_writeback,
};

struct inode *wjfs_get_inode(struct super_block *sb, int mode, dev_t dev)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct inode *inode = new_inode(sb);     // ??

	if(inode)
	{
		inode->i_mode = mode;
		inode->i_uid = current_fsuid();
		inode->i_gid = current_fsgid();
		inode->i_mapping->backing_dev_info = &wjfs_backing_dev_info;
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
		if(inode->i_ino)
			printk(KERN_EMERG"inode->i_ino : %d\n",inode->i_ino);
	}
	return inode;
}

//本地创建文件所使用的函数
static int wjfs_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t dev)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	int i = (int)atomic_read(&dentry->d_count);
	printk("wjfs_mknod i = %d\n", i);
	struct inode *inode = wjfs_get_inode(dir->i_sb, mode, dev);
	int err = -ENOSPC;

	if(inode)
	{
		if(dir->i_mode & S_ISGID)
		{
			inode->i_gid = dir->i_gid;
			if(S_ISDIR(mode))
			{
				inode->i_mode |= S_ISGID;
			}
		}
		//fill in inode information for a dentry
		d_instantiate(dentry, inode);
		//dput表示用户释放文件目录项dentry并将其放入缓冲池中，dget表示用户对dentry进行引用并增加它的引用计数。 
		dget(dentry);
		err = 0;
		dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	}
	//在本地创建一个文件的时候，进行通信，让其他端创建一个访问链接
	printk("开始创建远端文件链接\n");
	void *arg;
	wjfs_client_create(arg, &test_ip, test_port ,inode->i_ino, dentry->d_name.name,
						dentry->d_parent->d_inode->i_ino, dentry->d_parent->d_name.name, S_ISDIR(mode));
	return err;
}

// 远端创建文件使用的创建函数
static int wjfs_mknod_rdma(struct inode *dir, struct dentry *dentry, int mode, dev_t dev)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	int i = (int)atomic_read(&dentry->d_count);
	printk("wjfs_mknod i = %d\n", i);
	struct inode *inode = wjfs_get_inode(dir->i_sb, mode, dev);
	int err = -ENOSPC;

	if(inode)
	{
		if(dir->i_mode & S_ISGID)
		{
			inode->i_gid = dir->i_gid;
			if(S_ISDIR(mode))
			{
				inode->i_mode |= S_ISGID;
			}
		}
		//fill in inode information for a dentry
		d_instantiate(dentry, inode);
		//dput表示用户释放文件目录项dentry并将其放入缓冲池中，dget表示用户对dentry进行引用并增加它的引用计数。 
		dget(dentry);
		err = 0;
		dir->i_mtime = dir->i_ctime = CURRENT_TIME;
	}
	return err;
}

static int wjfs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	int retval = wjfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if(!retval)
		inc_nlink(dir);
	return retval;
}

static int wjfs_create(struct inode *dir, struct dentry *dentry, int mode, struct nameidata *nd)
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
	
	struct inode *inode = old_dentry->d_inode;
	int they_are_dirs = S_ISDIR(old_dentry->d_inode->i_mode);

	if (!simple_empty(new_dentry))
		return -ENOTEMPTY;

	if (new_dentry->d_inode) {
		simple_unlink(new_dir, new_dentry);
		if (they_are_dirs) {
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
static int simple_delete_dentry(struct dentry *dentry)
{
	return 1;
}
static struct dentry *wjfs_lookup(struct inode *dir, struct dentry *dentry, struct nameidata *nd)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	
	if(strcmp(dentry->d_name.name,"a")==0)
	{
		printk(KERN_EMERG" 获得a_dentry信息 \n");
		a_dentry = dentry;
	}
	
	static const struct dentry_operations simple_dentry_operations = {
		.d_delete = simple_delete_dentry,
	};
	if (dentry->d_name.len > NAME_MAX)
		return ERR_PTR(-ENAMETOOLONG);
	dentry->d_op = &simple_dentry_operations;
	d_add(dentry, NULL);
	return NULL;
}

//删除dir目录中的dentry目录项代表的文件   删除目录   删除操作不仅仅在这里，单纯屏蔽这个没有用
static int wjfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	//判断是否是我的文件
	if(is_my_file(dentry))
	{
		if (!simple_empty(dentry))
		return -ENOTEMPTY;
		drop_nlink(dentry->d_inode);
		simple_unlink(dir, dentry);
		drop_nlink(dir);
		//发送信息删除远端目录
		printk("开始删除远端文件链接\n");
		void *arg;
		wjfs_client_remove(arg, &test_ip, test_port ,dentry->d_inode->i_ino, dentry->d_name.name,
							dentry->d_parent->d_inode->i_ino, dentry->d_parent->d_name.name, 1);
	}
	else
	{
		printk("该文件是远端文件的链接,没有权限删除\n");
	}
	return 0;
}

//删除dir目录中的dentry目录项代表的文件   远端删除目录
static int wjfs_rmdir_rdma(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	if (!simple_empty(dentry))
		return -ENOTEMPTY;
	drop_nlink(dentry->d_inode);
	simple_unlink(dir, dentry);
	drop_nlink(dir);
	return 0;
}

// 符号链接的名称在dentry中，链接对象是dir目录中的名称为symname		此时作为调试的触发函数
static int wjfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	//test
	if(strcmp("server",dentry->d_name.name) == 0)
	{
		kthread_run(wjfs_server, NULL, "wjfs_server");
	}
	if(strcmp("table",dentry->d_name.name) == 0)
	{
		printk("开始打表了\n");
		int i = 0;
		for(i = 0; i < 1024; i++)
		{
			if(reg_ino_mapp_table[i].used == 1)
			{
				printk("reg_ino_mapp_table[%d].name: %s\n", i, reg_ino_mapp_table[i].name);
				printk("reg_ino_mapp_table[%d].local_ino: %d\n", i, reg_ino_mapp_table[i].local_ino);
				printk("reg_ino_mapp_table[%d].rdma_ino: %d\n", i, reg_ino_mapp_table[i].rdma_ino);
				printk("reg_ino_mapp_table[%d].local_parent_ino: %d\n", i, reg_ino_mapp_table[i].local_parent_ino);
				printk("reg_ino_mapp_table[%d].rdma_parent_ino: %d\n", i, reg_ino_mapp_table[i].rdma_parent_ino);
				printk("reg_ino_mapp_table[%d].ip: %s\n", i, reg_ino_mapp_table[i].ip);
			} 
		}
	}

	printk("wjfs_symlink over\n");
	return 0;
}

//硬链接名称由dentry参数指定，链接对象是dir目录中old_dentry目录项所代表的文件
static int wjfs_link(struct dentry *old_dentry, struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct inode *inode = old_dentry->d_inode;

	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	inc_nlink(inode);
	atomic_inc(&inode->i_count);
	dget(dentry);
	d_instantiate(dentry, inode);
	return 0;
}

//从目录dir中删除dentry指定的索引节点对象   删除文件    删除操作不仅仅在这里，单纯屏蔽这个没有用
static int wjfs_unlink(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	//判断是否是我的文件
	if(is_my_file(dentry))
	{
		struct inode *inode = dentry->d_inode;
		inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
		drop_nlink(inode);
		dput(dentry);
		//发送信息删除远端目录
		printk("开始删除远端文件链接\n");
		void *arg;
		wjfs_client_remove(arg, &test_ip, test_port ,dentry->d_inode->i_ino, dentry->d_name.name,
							dentry->d_parent->d_inode->i_ino, dentry->d_parent->d_name.name, 0);
	}
	else
	{
		printk("该文件是远端文件的链接,没有权限删除\n");
	}
	return 0;
}

//从目录dir中删除dentry指定的索引节点对象   远端删除文件
static int wjfs_unlink_rdma(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct inode *inode = dentry->d_inode;
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
};

//用于超级块的相关操作
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
		}
	}
	return 0;
}

int wjfs_fill_super(struct super_block *sb, void *data, int silent)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	
	struct wjfs_fs_info *fsi;
	struct inode *root_inode;
	struct dentry *root_dentry;
	int err = 0;
	save_mount_options(sb, data);

	// s_fs_info 文件系统特殊信息
	fsi = kzalloc(sizeof(struct wjfs_fs_info),GFP_KERNEL);
	sb->s_fs_info = fsi;
	if(!fsi)
	{
		err = -ENOMEM;
		goto fail;
	}
	printk("fsi = %d\n",fsi);

	err = wjfs_parse_options(data, &fsi->mount_opts);
	if(err)
		goto fail;

	//一些其他属性的设置
	sb->s_maxbytes                 = MAX_LFS_FILESIZE;        //文件大小的上限
	sb->s_blocksize                = PAGE_SIZE;
	sb->s_blocksize_bits           = PAGE_SHIFT;
	sb->s_magic                    = RAMFS_MAGIC;             // ??
	sb->s_op                       = &wjfs_ops;
	sb->s_time_gran                = 1;                       //时间戳粒度

	root_inode = wjfs_get_inode(sb, S_IFDIR | fsi->mount_opts.mode, 0);
	if(!root_inode)
	{
		err = -ENOMEM;
		goto fail;
	}
	root_dentry = d_alloc_root(root_inode);
	sb->s_root = root_dentry;                 //s_root 目录挂载点

	if(!root_dentry)
	{
		err = -ENOMEM;
		goto fail;
	}
	
	//通过全局变量将三个导出来方便使用
	rroot_sb = sb;
	rroot_dentry = sb->s_root;
	rroot_inode = sb->s_root->d_inode;

	//初始化映射表
	int i = 0;
	for(i = 0; i < 1024; i++)
	{
		reg_ino_mapp_table[i].used = 0;
	}

	return 0;

fail:
	kfree(fsi);
	sb->s_fs_info = NULL;
	iput(root_inode);
	return err;
}

int wjfs_get_sb(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data, struct vfsmount *mnt)
{
	return get_sb_nodev(fs_type, flags, data, wjfs_fill_super, mnt);
}

//自制卸载文件系统函数
//迭代查找dentry，然后把d_count置0
void wjfs_d_genocide(struct dentry *root)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct dentry *this_parent = root;
	struct list_head *next;

	spin_lock(&dcache_lock);
repeat:
	next = this_parent->d_subdirs.next;
resume:
	while (next != &this_parent->d_subdirs) 
	{
		struct list_head *tmp = next;
		struct dentry *dentry = list_entry(tmp, struct dentry, d_u.d_child);
		next = tmp->next;
		if (d_unhashed(dentry)||!dentry->d_inode)
			continue;
		if (!list_empty(&dentry->d_subdirs)) {
			this_parent = dentry;
			goto repeat;
		}
		atomic_dec(&dentry->d_count);
	}
	if (this_parent != root) 
	{
		next = this_parent->d_u.d_child.next;
		atomic_dec(&this_parent->d_count);
		this_parent = this_parent->d_parent;
		goto resume;
	}
	spin_unlock(&dcache_lock);
}

void wjfs_kill_litter_super(struct super_block *sb)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	if (sb->s_root)
		wjfs_d_genocide(sb->s_root);
	kill_anon_super(sb);
}

void wjfs_kill_sb(struct super_block *sb)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	/* 卸载一个文件系统时要做的操作 */
	kfree(sb->s_fs_info);
	wjfs_kill_litter_super(sb);
}

static struct file_system_type wjfs_fs_type = {
	.name           = "wjfs",
	.get_sb         = wjfs_get_sb,              // mount -t wjfs wjfs /mnt 执行此函数
	.kill_sb        = wjfs_kill_sb,
};

static int __init init_wjfs(void)
{
	printk("register_filesystem\n");
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);

	int retval;
	retval = register_filesystem(&wjfs_fs_type);
	kthread_run(wjfs_server, NULL, "wjfs_server");
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
static void wait_on_retry_sync_kiocb(struct kiocb *iocb)
{
	set_current_state(TASK_UNINTERRUPTIBLE);
	if (!kiocbIsKicked(iocb))
		schedule();
	else
		kiocbClearKicked(iocb);
	__set_current_state(TASK_RUNNING);
}

//每次cat a 执行这个函数两次，第二次不实际调用 wjfs_file_read_actor
ssize_t wjfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)  
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	printk("len : %d\n", len);  // always 32768
	struct iovec iov = { .iov_base = buf, .iov_len = len };
	struct kiocb kiocb;
	ssize_t ret = 0;
	init_sync_kiocb(&kiocb, filp);
	kiocb.ki_pos = *ppos;
	kiocb.ki_left = len;
	struct dentry *dentry = filp->f_dentry;
	char message[1024];

	//在读之前首先要判断是否是本地文件
	if(is_my_file(dentry))
	{
		for (;;) 
		{
			ret = filp->f_op->aio_read(&kiocb, &iov, 1, kiocb.ki_pos);
			if (ret != -EIOCBRETRY)
				break;
			wait_on_retry_sync_kiocb(&kiocb);
		}

		if (-EIOCBQUEUED == ret)
			ret = wait_on_sync_kiocb(&kiocb);
		*ppos = kiocb.ki_pos;
	}
	else   //要读取的是远端文件
	{
		static int k = 1;
		printk("开始远端读取文件信息\n");
		void *arg;
		memset(message, 0, sizeof(char)*1024);
		wjfs_client_read(arg, &test_ip, test_port ,dentry->d_inode->i_ino, dentry->d_name.name,
							dentry->d_parent->d_inode->i_ino, dentry->d_parent->d_name.name, message);
		printk("读取到的远端信息是：%s\n", message);
		memset(buf, 0, sizeof(char)*1024);
		strcpy(buf, message);
		if(k%2)
			ret = strlen(buf);
		else
			ret = 0;
		k++;
	}
	
	return ret;  //读取到的字节数，即显示在屏幕上的字节数
}

//对inode进行写的时候，是没法进行读操作的
ssize_t wjfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct iovec iov = { .iov_base = (void __user *)buf, .iov_len = len };  //把要写的信息填在这个buf中就好了
	struct kiocb kiocb;
	ssize_t ret = 0;
	struct dentry *dentry = filp->f_dentry;
	char message[1024];

	//在写之前，首先要判断信息要写到哪个地方去,判断文件是否是远端的
	if(is_my_file(filp->f_dentry))
	{
		init_sync_kiocb(&kiocb, filp);
		kiocb.ki_pos = *ppos;
		kiocb.ki_left = len;

		for (;;) {
			ret = filp->f_op->aio_write(&kiocb, &iov, 1, kiocb.ki_pos);
			if (ret != -EIOCBRETRY)
				break;
			wait_on_retry_sync_kiocb(&kiocb);
		}

		if (-EIOCBQUEUED == ret)
			ret = wait_on_sync_kiocb(&kiocb);
		*ppos = kiocb.ki_pos;
	}
	else   //该文件是远端文件
	{
		printk("开始远端写入信息\n");
		memset(message, 0, sizeof(char)*1024);
		int i = 0;
		for(i = 0; i < len; i++)
			message[i] = buf[i];
		printk("要远程写入的信息是：%s\n", message);
		void *arg;
		wjfs_client_write(arg, &test_ip, test_port ,dentry->d_inode->i_ino, dentry->d_name.name,
							dentry->d_parent->d_inode->i_ino, dentry->d_parent->d_name.name, message);
		ret = strlen(message);
	}
	
	return ret;
}

const struct file_operations wjfs_file_operations = {
	.read       = wjfs_read,
	.aio_read	= generic_file_aio_read,
	.write      = wjfs_write,
	.aio_write	= generic_file_aio_write,
	.mmap		= generic_file_mmap,
	.fsync		= simple_sync_file,
	.splice_read	= generic_file_splice_read,
	.splice_write	= generic_file_splice_write,
	.llseek		= generic_file_llseek,
};

const struct inode_operations wjfs_file_inode_operations = {
	.getattr	= simple_getattr,
};

//服务端创建文件广播创建信息的函数,（目前是点对点的）
//ino 为远端创建文件的inode->i_ino
//name 为远端创建文件的名称
//ino_parent 为远端创建文件的父目录的 inode->i_ino
//name_parent 为远端创建文件的父目录的名称
//mode 标记是文件还是文件夹
static int wjfs_client_create(void *arg, char *ip, int port ,int ino, char *name,int ino_parent, char *name_parent, int mode)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);

	printk("要发往数据的ip地址是: %s\t端口号是: %d\n", ip, port);
	printk("要创建文件的名称是:%s\t节点号是:%d\t父目录名称是%s\t节点号是%d\n", name, ino, name_parent, ino_parent);
	if(mode)
		printk("我是目录文件\n");
	else
		printk("我是普通文件\n");

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
	addr_srv.sin_port = htons(port);
	addr_srv.sin_addr.s_addr = inet_addr(ip);
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

	//创建发送的字符串
	memset(buf, 0, sizeof(char)*1024);
	if(mode)
		len = sprintf(buf, "%s %d %s %d %s %s","CR", ino, name, ino_parent, name_parent, "1111");
	else
		len = sprintf(buf, "%s %d %s %d %s %s","CR", ino, name, ino_parent, name_parent, "0000");
	printk("创建远程文件链接所要传输的数据是: %s\n", buf);
	ksend(sockfd_cli, buf, len, 0);

	kclose(sockfd_cli);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}

//服务端删除文件广播创建信息的函数,（目前是点对点的）
//ino 为远端删除文件的inode->i_ino
//name 为远端删除文件的名称
//ino_parent 为远端删除文件的父目录的 inode->i_ino
//name_parent 为远端删除文件的父目录的名称
//mode 标记是文件还是文件夹
static int wjfs_client_remove(void *arg, char *ip, int port ,int ino, char *name,int ino_parent, char *name_parent, int mode)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);

	printk("要发往数据的ip地址是: %s\t端口号是: %d\n", ip, port);
	printk("要删除文件的名称是:%s\t节点号是:%d\t父目录名称是%s\t节点号是%d\n", name, ino, name_parent, ino_parent);
	if(mode)
		printk("我是目录文件\n");
	else
		printk("我是普通文件\n");

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
	addr_srv.sin_port = htons(port);
	addr_srv.sin_addr.s_addr = inet_addr(ip);
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

	//创建发送的字符串
	memset(buf, 0, sizeof(char)*1024);
	if(mode)
		len = sprintf(buf, "%s %d %s %d %s %s","RM", ino, name, ino_parent, name_parent, "1111");
	else
		len = sprintf(buf, "%s %d %s %d %s %s","RM", ino, name, ino_parent, name_parent, "0000");
	printk("删除远程文件链接所要传输的数据是: %s\n", buf);
	ksend(sockfd_cli, buf, len, 0);

	kclose(sockfd_cli);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}

//服务端写入信息文件广播创建信息的函数,（目前是点对点的）
//ino 为远端写入信息文件的inode->i_ino
//name 为远端写入信息文件的名称
//ino_parent 为远端写入信息文件的父目录的 inode->i_ino
//name_parent 为远端写入信息文件的父目录的名称
//message 为要远程写入的信息
static int wjfs_client_write(void *arg, char *ip, int port ,int ino, char *name,int ino_parent, char *name_parent, char *message)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);

	printk("要发往数据的ip地址是: %s\t端口号是: %d\n", ip, port);
	printk("要写入信息文件的名称是:%s\t节点号是:%d\t父目录名称是%s\t节点号是%d\n", name, ino, name_parent, ino_parent);

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
	addr_srv.sin_port = htons(port);
	addr_srv.sin_addr.s_addr = inet_addr(ip);
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

	//需要先通过表获取需要写入信息的文件的远端节点号和远端父节点号
	int my_rdma_ino = 0, my_rdma_parent_ino = 0, i = 0;
	for(i = 0; i < 1024; i++)
	{
		if(reg_ino_mapp_table[i].used == 1)
		{
			if((reg_ino_mapp_table[i].local_ino == ino) && (reg_ino_mapp_table[i].local_parent_ino == ino_parent))
			{
				my_rdma_ino = reg_ino_mapp_table[i].rdma_ino;
				my_rdma_parent_ino = reg_ino_mapp_table[i].rdma_parent_ino;
				break;
			}
		}
	}

	//创建发送的字符串
	memset(buf, 0, sizeof(char)*1024);
	len = sprintf(buf, "%s %d %s %d %s %s","WR", my_rdma_ino, name, my_rdma_parent_ino, name_parent, message);
	printk("写入信息文件链接所要传输的数据是: %s\n", buf);
	ksend(sockfd_cli, buf, len, 0);

	kclose(sockfd_cli);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}

//服务端读取信息文件广播创建信息的函数,（目前是点对点的）
//ino 为远端读取信息文件的inode->i_ino
//name 为远端读取信息文件的名称
//ino_parent 为远端读取信息文件的父目录的 inode->i_ino
//name_parent 为远端读取信息文件的父目录的名称
//message 为要远程读取的信息
static int wjfs_client_read(void *arg, char *ip, int port ,int ino, char *name,int ino_parent, char *name_parent, char *message)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);

	printk("要读取数据的ip地址是: %s\t端口号是: %d\n", ip, port);
	printk("要读取信息文件的名称是:%s\t节点号是:%d\t父目录名称是%s\t节点号是%d\n", name, ino, name_parent, ino_parent);

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
	addr_srv.sin_port = htons(port);
	addr_srv.sin_addr.s_addr = inet_addr(ip);
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

	//需要先通过表获取需要读取信息的文件的远端节点号和远端父节点号
	int my_rdma_ino = 0, my_rdma_parent_ino = 0, i = 0;
	for(i = 0; i < 1024; i++)
	{
		if(reg_ino_mapp_table[i].used == 1)
		{
			if((reg_ino_mapp_table[i].local_ino == ino) && (reg_ino_mapp_table[i].local_parent_ino == ino_parent))
			{
				my_rdma_ino = reg_ino_mapp_table[i].rdma_ino;
				my_rdma_parent_ino = reg_ino_mapp_table[i].rdma_parent_ino;
				break;
			}
		}
	}

	//创建发送的字符串
	memset(buf, 0, sizeof(char)*1024);
	len = sprintf(buf, "%s %d %s %d %s","RD", my_rdma_ino, name, my_rdma_parent_ino, name_parent);
	printk("读取信息文件链接所要传输的数据是: %s\n", buf);
	ksend(sockfd_cli, buf, len, 0);

	//接收远端回传的信息
	memset(message, 0, sizeof(char)*1024);
	krecv(sockfd_cli, message, 1024, 0);
	printk("接收远端回传的信息是: %s\n", message);

	kclose(sockfd_cli);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}

static int wjfs_server(void *arg)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	ksocket_t sockfd_srv, sockfd_cli;
	struct sockaddr_in addr_srv;
	struct sockaddr_in addr_cli;
	char buf[1024];
	char message[1024];
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
	addr_srv.sin_port = htons(test_port);
	addr_srv.sin_addr.s_addr = INADDR_ANY;
	addr_len = sizeof(struct sockaddr_in);

	sockfd_srv = ksocket(AF_INET, SOCK_STREAM, 0);
	printk("sockfd_srv = 0x%p\n", sockfd_srv);
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

	//开始持续监听
	while(1)
	{
		sockfd_cli = kaccept(sockfd_srv, (struct sockaddr *)&addr_cli, &addr_len);
		if(sockfd_cli == NULL)
		{
			printk("kaccept failed\n");
			return -1;
		}
		else
		{
			printk("kaccept ok\n");
			printk("sockfd_cli = 0x%p\n", sockfd_cli);
		}

		//用于输出客户端的ip和端口号信息
		tmp = inet_ntoa(&addr_cli.sin_addr);
		printk("w1 server got connected from : %s %d\n", tmp, ntohs(addr_cli.sin_port));

		memset(buf, 0, sizeof(char)*1024);
		memset(message, 0, sizeof(char)*1024);
		len = krecv(sockfd_cli, buf, sizeof(buf), 0);
		printk("w1 server got message: %s\n", buf);   //拿到的是要访问的文件的相关信息

		if (memcmp(buf, "quit", 4) == 0)  //只比较前4个字节
			break;

		//按照拿到的信息，首先判断是 创建，读取，写入，删除 文件
		if(memcmp(buf, "RD", 2) == 0)   //读文件
		{
			printk("远端信息指示读取一个文件的信息\n");
			rdma_read(buf);
			printk("本地读取到的数据是：%s\n", buf);
			ksend(sockfd_cli, buf, strlen(buf), 0);
		}
		if(memcmp(buf, "WR", 2) == 0)   //写文件
		{
			printk("远端信息指示将信息写入一个文件\n");
			rdma_write(buf);
		}
		if(memcmp(buf, "CR", 2) == 0)   //创建一个新文件
		{
			printk("远端信息指示创建一个新的文件\n");
			rdma_create(buf, tmp);
		}
		if(memcmp(buf, "RM", 2) == 0)   //删除一个文件
		{
			printk("远端信息指示删除一个文件\n");
			rdma_remove(buf);
		}

		kfree(tmp);
		kclose(sockfd_cli);
	}

	kclose(sockfd_srv);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}

//根据拿到的远端信息创建一个文件
int rdma_create(char *buf, char *ip)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	//拿到的信息就是要创建的文件的相关信息,扫描全部的dentry要创建文件的父目录的inode
	int ino, ino_parent;
	char name[20], name_parent[20];
	int mode;

	char tmp_name[6][20];
	char *tmp = buf;
	int i = 0, j = 0, k = 0;
	for(i = 0; i < strlen(tmp); i++)
	{
		k = 0;
		while((!(tmp[i] == 32 || tmp[i] == 13 || tmp[i] == 10))&&(i < strlen(tmp)))
		{
			tmp_name[j][k++] = tmp[i++];
		}
		tmp_name[j][k] = '\0';
		j++;
	}
	for(i = 0; i < 6; i++)
	{
		printk("name %d: %s\n", i, tmp_name[i]);
	}
	ino = string2int(tmp_name[1]);
	ino_parent = string2int(tmp_name[3]);
	mode = string2int(tmp_name[5]);
	strcpy(name, tmp_name[2]);
	strcpy(name_parent, tmp_name[4]);

	printk("将要创建文件的父目录名称: %s\n", name_parent);
	printk("将要创建文件的父目录节点号: %d\n", ino_parent);
	printk("将要创建文件的名称: %s\n", name);
	printk("将要创建文件的节点号: %d\n", ino);
	if(mode)
		printk("将要创建文件的属性: 目录文件, mode: %d\n", mode);
	else
		printk("将要创建文件的属性: 普通文件, mode: %d\n", mode);
	
	//获取信息成功
	struct dentry *tmp_dentry = rroot_dentry;  //从根节点开始遍历所有的dentry
	//迭代获取目标dentry
	printk(KERN_EMERG"目录名称 %s\n",tmp_dentry->d_name.name);
	if(!strcmp(tmp_dentry->d_name.name, name_parent))
	{
		printk("父目录就是根目录\n");
	}
	else
	{
		tmp_dentry = find_from_all_dentry(tmp_dentry, name_parent);
	}
	//tmp_dentry 为获得的父目录的dentry
	printk(KERN_EMERG"tmp_dentry %s\n",tmp_dentry->d_name.name);
	printk(KERN_EMERG"tmp_dentry->d_inode->i_ino %d\n",tmp_dentry->d_inode->i_ino);
	//创建远程的链接访问文件
	if(mode)
	{
		struct dentry *new;
		new = wjfs_create_dir(name, tmp_dentry);   //目录创建完成之后就可以了
		printk("目录创建完成\n");
		//目录创建完成后把dentry的引用量减 1
		atomic_dec(&new->d_count);
		//文件创建完成之后还需要将信息写入节点映射表中
		int i = 0;
		for(i = 0; i < 1024; i++)
		{
			if(reg_ino_mapp_table[i].used == 0)
			{
				reg_ino_mapp_table[i].used = 1;
				break;
			}
		}
		strcpy(reg_ino_mapp_table[i].name, new->d_name.name);
		reg_ino_mapp_table[i].local_ino = new->d_inode->i_ino;
		reg_ino_mapp_table[i].rdma_ino = ino;
		reg_ino_mapp_table[i].local_parent_ino = tmp_dentry->d_inode->i_ino;
		reg_ino_mapp_table[i].rdma_parent_ino = ino_parent;
		strcpy(reg_ino_mapp_table[i].ip, ip);
	}
	else
	{
		struct dentry *new;
		new = wjfs_create_file(name, S_IFREG | S_IRUGO , tmp_dentry, NULL, NULL);
		printk("文件创建完成\n");
		//文件创建完成后把dentry的引用量减 1
		atomic_dec(&new->d_count);
		//文件创建完成之后还需要将信息写入节点映射表中
		int i = 0;
		for(i = 0; i < 1024; i++)
		{
			if(reg_ino_mapp_table[i].used == 0)
			{
				reg_ino_mapp_table[i].used = 1;
				break;
			}
		}
		strcpy(reg_ino_mapp_table[i].name, new->d_name.name);
		reg_ino_mapp_table[i].local_ino = new->d_inode->i_ino;
		reg_ino_mapp_table[i].rdma_ino = ino;
		reg_ino_mapp_table[i].local_parent_ino = tmp_dentry->d_inode->i_ino;
		reg_ino_mapp_table[i].rdma_parent_ino = ino_parent;
		strcpy(reg_ino_mapp_table[i].ip, ip);
	}
	return 0;
}

//根据拿到的远端信息删除一个文件
int rdma_remove(char *buf)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	//拿到的信息就是要删除的文件的相关信息
	int ino, ino_parent;
	char name[20], name_parent[20];
	int mode;

	char tmp_name[6][20];
	char *tmp = buf;
	int i = 0, j = 0, k = 0;
	for(i = 0; i < strlen(tmp); i++)
	{
		k = 0;
		while((!(tmp[i] == 32 || tmp[i] == 13 || tmp[i] == 10))&&(i < strlen(tmp)))
		{
			tmp_name[j][k++] = tmp[i++];
		}
		tmp_name[j][k] = '\0';
		j++;
	}
	for(i = 0; i < 6; i++)
	{
		printk("name %d: %s\n", i, tmp_name[i]);
	}
	ino = string2int(tmp_name[1]);
	ino_parent = string2int(tmp_name[3]);
	mode = string2int(tmp_name[5]);
	strcpy(name, tmp_name[2]);
	strcpy(name_parent, tmp_name[4]);

	printk("将要删除文件的父目录名称: %s\n", name_parent);
	printk("将要删除文件的父目录节点号: %d\n", ino_parent);
	printk("将要删除文件的名称: %s\n", name);
	printk("将要删除文件的节点号: %d\n", ino);
	if(mode)
		printk("将要删除文件的属性: 目录文件, mode: %d\n", mode);
	else
		printk("将要删除文件的属性: 普通文件, mode: %d\n", mode);

	//需要先通过表获取需要删除的文件的本地节点号和本地父节点号
	int my_ino = 0, my_parent_ino = 0;
	for(i = 0; i < 1024; i++)
	{
		if(reg_ino_mapp_table[i].used == 1)
		{
			if((reg_ino_mapp_table[i].rdma_ino == ino) && (reg_ino_mapp_table[i].rdma_parent_ino == ino_parent))
			{
				my_ino = reg_ino_mapp_table[i].local_ino;
				my_parent_ino = reg_ino_mapp_table[i].local_parent_ino;
				reg_ino_mapp_table[i].used = 0;
				break;
			}
		}
	}
	printk("要删除的文件本地节点号是： %d\n", my_ino);
	printk("要删除的文件本地的父节点号是： %d\n", my_parent_ino);
	
	//获取要删除文件的dentry
	struct dentry *tmp_dentry = rroot_dentry;  //从根节点开始遍历所有的dentry
	//迭代获取目标dentry
	tmp_dentry = find_dentry_by_ino(tmp_dentry, my_ino);
	//tmp_dentry 为要删除文件的目录
	if(tmp_dentry)
	{
		printk(KERN_EMERG"tmp_dentry %s\n",tmp_dentry->d_name.name);
		printk(KERN_EMERG"tmp_dentry->d_inode->i_ino %d\n",tmp_dentry->d_inode->i_ino);
		//删除远程的链接访问文件
		if(mode)
		{
			//删除目录
			wjfs_rmdir_rdma(tmp_dentry->d_parent->d_inode, tmp_dentry);
		}
		else
		{
			//删除文件
			wjfs_unlink_rdma(tmp_dentry->d_parent->d_inode, tmp_dentry);
		}
	}
	else
	{
		printk("没有找到要删除文件的dentry\n");
	}
	return 0;
}

//根据拿到的远端信息往一个文件中写入信息
int rdma_write(char *buf)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	//拿到的信息就是要写入信息的文件的相关信息
	int ino, ino_parent;
	char name[20], name_parent[20];
	char message[1024];

	char tmp_name[6][20];
	char *tmp = buf;
	int i = 0, j = 0, k = 0;
	for(i = 0; i < strlen(tmp); i++)
	{
		k = 0;
		while((!(tmp[i] == 32 || tmp[i] == 13 || tmp[i] == 10))&&(i < strlen(tmp)))
		{
			tmp_name[j][k++] = tmp[i++];
		}
		tmp_name[j][k] = '\0';
		j++;
		if(j == 5)
		{
			i++;
			break;
		}
	}
	for(k = 0; i < strlen(tmp); i++)
	{
		while((!(tmp[i] == 13 || tmp[i] == 10))&&(i < strlen(tmp)))
		{
			tmp_name[j][k++] = tmp[i++];
		}
		tmp_name[j][k] = '\0';
	}
	for(i = 0; i < 6; i++)
	{
		printk("name %d: %s\n", i, tmp_name[i]);
	}
	ino = string2int(tmp_name[1]);
	ino_parent = string2int(tmp_name[3]);
	strcpy(name, tmp_name[2]);
	strcpy(name_parent, tmp_name[4]);
	strcpy(message, tmp_name[5]);

	printk("将要写入信息的文件的父目录名称: %s\n", name_parent);
	printk("将要写入信息的文件的父目录节点号: %d\n", ino_parent);
	printk("将要写入信息的文件的名称: %s\n", name);
	printk("将要写入信息的文件的节点号: %d\n", ino);
	printk("将要写入文件的信息是：%s\n", message);
	
	//获取要写入信息的文件的dentry
	struct dentry *tmp_dentry = rroot_dentry;  //从根节点开始遍历所有的dentry
	//迭代获取目标dentry
	tmp_dentry = find_dentry_by_ino(tmp_dentry, ino);
	//tmp_dentry 为要写入信息的文件的目录
	if(tmp_dentry)
	{
		printk(KERN_EMERG"tmp_dentry %s\n",tmp_dentry->d_name.name);
		printk(KERN_EMERG"tmp_dentry->d_inode->i_ino %d\n",tmp_dentry->d_inode->i_ino);
		//写入信息
		put_file_message(tmp_dentry, message);
	}
	else
	{
		printk("没有找到要删除文件的dentry\n");
	}
	return 0;
}

//根据拿到的信息读取一个文件的信息并发送回去
int rdma_read(char *buf)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	//拿到的信息就是要读取信息的文件的相关信息
	int ino, ino_parent;
	char name[20], name_parent[20];
	char message[1024];

	char tmp_name[5][20];
	char *tmp = buf;
	int i = 0, j = 0, k = 0;
	for(i = 0; i < strlen(tmp); i++)
	{
		k = 0;
		while((!(tmp[i] == 32 || tmp[i] == 13 || tmp[i] == 10))&&(i < strlen(tmp)))
		{
			tmp_name[j][k++] = tmp[i++];
		}
		tmp_name[j][k] = '\0';
		j++;
	}
	for(i = 0; i < 5; i++)
	{
		printk("name %d: %s\n", i, tmp_name[i]);
	}
	ino = string2int(tmp_name[1]);
	ino_parent = string2int(tmp_name[3]);
	strcpy(name, tmp_name[2]);
	strcpy(name_parent, tmp_name[4]);

	printk("将要读取信息的文件的父目录名称: %s\n", name_parent);
	printk("将要读取信息的文件的父目录节点号: %d\n", ino_parent);
	printk("将要读取信息的文件的名称: %s\n", name);
	printk("将要读取信息的文件的节点号: %d\n", ino);
	
	//获取要读取信息的文件的dentry
	struct dentry *tmp_dentry = rroot_dentry;  //从根节点开始遍历所有的dentry
	//迭代获取目标dentry
	tmp_dentry = find_dentry_by_ino(tmp_dentry, ino);
	//tmp_dentry 为要读取信息的文件的目录
	if(tmp_dentry)
	{
		printk(KERN_EMERG"tmp_dentry %s\n",tmp_dentry->d_name.name);
		printk(KERN_EMERG"tmp_dentry->d_inode->i_ino %d\n",tmp_dentry->d_inode->i_ino);
		//写入信息
		get_file_message(tmp_dentry, message);
		memset(buf, 0, sizeof(char)*1024);
		strcpy(buf, message);
	}
	else
	{
		printk("没有找到要删除文件的dentry\n");
	}
}

//获取dentry代表的文件的信息
int get_file_message(struct dentry *dentry, char *message)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);

	pgoff_t index = 0;
	unsigned long offset = 0;
	struct page *page;
	unsigned long nr, ret;
	pgoff_t end_index;
	loff_t isize;
	char *kaddr;
	unsigned long left;
	char buf[1024];        //一定要记得开辟空间

	// 拿到了正确的inode与dentry，加下来读取文件中的信息
	page = find_get_page(dentry->d_inode->i_mapping, index);
	if(page)
	{
		printk("page %d\n",page);
	}
	printk(KERN_EMERG"dentry->d_inode->i_ino %d\n",dentry->d_inode->i_ino);
	isize = i_size_read(dentry->d_inode);    //对应inode中的信息数量+1
	printk("isize %d\n",isize);
	if(isize == 0)
	{
		printk("无可读信息\n");
		return 0;
	}
	else
	{
		end_index = (isize - 1) >> PAGE_CACHE_SHIFT;
		nr = PAGE_CACHE_SIZE;
		if (index == end_index)
		{
			nr = ((isize - 1) & ~PAGE_CACHE_MASK) + 1;
		}
		nr = nr - offset;
		kaddr = kmap_atomic(page, KM_USER0);
		left = __copy_to_user_inatomic(buf, kaddr + offset, nr);
		kunmap_atomic(kaddr, KM_USER0);
		printk("buf : %s\n", buf);
		int buf_len = strlen(buf);
		printk("buf_len : %d\n",buf_len);
		strcpy(message, buf);
	}
	return 0;
}

//将buf中的内容写入dentry代表的文件中
int put_file_message(struct dentry *dentry, char *message)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct iovec iov = { .iov_base = message, .iov_len = strlen(message) };
	struct kiocb kiocbb;
	struct file filep;
	struct file *filp = &filep;
	struct address_space *mapping = dentry->d_inode->i_mapping;
	struct inode *inode = dentry->d_inode;
	struct iov_iter i;
	struct page *page;
	pgoff_t index;
	unsigned long offset;
	unsigned long bytes;
	size_t copied;
	void *fsdata;
	loff_t pos = 0;
	char *kaddr;

	init_sync_kiocb(&kiocbb, filp);
	kiocbb.ki_pos = 0;
	kiocbb.ki_left = strlen(message);

	iov_iter_init(&i, &iov, 1, 0, 0);

	offset = (pos & (PAGE_CACHE_SIZE - 1));
	index = pos >> PAGE_CACHE_SHIFT;
	//bytes = min_t(unsigned long, PAGE_CACHE_SIZE - offset, iov_iter_count(&i));
	bytes = strlen(message);

	simple_write_begin(filp, mapping, pos, bytes, 0, &page, &fsdata);
	kaddr = kmap_atomic(page, KM_USER0);
	if (likely(i.nr_segs == 1)) 
	{
		int left;
		char __user *buf = i.iov->iov_base + i.iov_offset;
		printk("buf: %s\n", buf);
		printk("bytes: %d\n", bytes);
		left = __copy_from_user_inatomic(kaddr + offset, buf, bytes);
		printk("left: %d\n", left);
		copied = bytes - left;
	}
	kunmap_atomic(kaddr, KM_USER0);
	simple_write_end(filp, mapping, pos, bytes, copied, page, fsdata);

	return 0;
}

int string2int(char *buf)
{
	int ans = 0;
	int i = 0;
	for(i = 0; i < strlen(buf); i++)
	{
		ans = ans * 10;
		ans = ans + (buf[i] - '0');
	}
	return ans;
}

//在系统内部自动创建文件的函数
static int wjfs_create_by_name(const char *name, mode_t mode, struct dentry *parent, struct dentry **dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	int error = 0;
	if(!parent)
	{
		parent = rroot_dentry;
	}
	*dentry = NULL;
	*dentry = lookup_one_len(name, parent, strlen(name));

	int i = (int)atomic_read(&((*dentry)->d_count));
	printk("%s = %d\n",name, i);
	
	if(!IS_ERR(dentry))
	{
		if((mode & S_IFMT) == S_IFDIR)
			error = wjfs_mknod_rdma(parent->d_inode, *dentry, mode | S_IFDIR, 0);
		else
			error = wjfs_mknod_rdma(parent->d_inode, *dentry, mode | S_IFREG, 0);
	}
	else
	{
		error = PTR_ERR(dentry);
	}
	//在内核下创建完成之后需要将每一个dentry引用量减 1
	//atomic_dec(&((*dentry)->d_count));
	return error;
}
struct dentry *wjfs_create_file(const char *name, mode_t mode, struct dentry *parent, void *data, struct file_operations *fops)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct dentry *dentry = NULL;
	int error;
	error = wjfs_create_by_name(name, mode, parent, &dentry);
	if(error)
	{
		dentry = NULL;
		goto exit;
	}
	if(dentry->d_inode)
	{
		if(data)
			//dentry->d_inode->generic_ip = data;
		if(fops)
			dentry->d_inode->i_fop = fops;
	}
	exit:
		return dentry;
}
struct dentry *wjfs_create_dir(const char *name, struct dentry *parent)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	return wjfs_create_file(name, S_IFDIR | S_IRWXU | S_IRUGO | S_IXUGO, parent, NULL, NULL);
}

//设计成递归查找所有的dentry  根据名称查找，用于创建文件
struct dentry *find_from_all_dentry(struct dentry *dentry, char *name_parent)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct dentry *ans_dentry;
	struct list_head *plist;
	struct dentry *sub_dentry;
	//这么做不能检查到所有的dentry，需要设计成递归
	list_for_each(plist, &(dentry->d_subdirs))
	{
		sub_dentry = list_entry(plist, struct dentry, d_u.d_child);

		if(sub_dentry->d_inode == NULL)
		{
			continue;
		}
		if(sub_dentry->d_inode->i_mode == NULL)
		{
			continue;
		}
		if(S_ISDIR(sub_dentry->d_inode->i_mode))
		{
			ans_dentry = find_from_all_dentry(sub_dentry, name_parent);
			if(ans_dentry && !strcmp(ans_dentry->d_name.name, name_parent))
			{
				return ans_dentry;
			}
		}
		
		if(sub_dentry)
		{
			printk(KERN_EMERG"subdirs_dentry %s\n",sub_dentry->d_name.name);
			printk(KERN_EMERG"sub_dentry->d_inode->i_ino %d\n",sub_dentry->d_inode->i_ino);
		}

		if(!strcmp(sub_dentry->d_name.name,name_parent))
		{
			return sub_dentry;
		}
	}
	return NULL;
}

//根据ino号查找文件dentry
struct dentry *find_dentry_by_ino(struct dentry *dentry, int ino)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct dentry *ans_dentry;
	struct list_head *plist;
	struct dentry *sub_dentry;
	//这么做不能检查到所有的dentry，需要设计成递归
	printk("1\n");
	list_for_each(plist, &(dentry->d_subdirs))
	{
		printk("2\n");
		sub_dentry = list_entry(plist, struct dentry, d_u.d_child);
		printk("3\n");
		if(sub_dentry->d_inode == NULL)
		{
			printk("4\n");
			continue;
		}
		if(sub_dentry->d_inode->i_mode == NULL)
		{
			printk("5\n");
			continue;
		}
		if(S_ISDIR(sub_dentry->d_inode->i_mode))
		{
			printk("6\n");
			ans_dentry = find_dentry_by_ino(sub_dentry, ino);
			printk("7\n");
			if(ans_dentry && (ans_dentry->d_inode->i_ino == ino))
			{
				printk("8\n");
				return ans_dentry;
			}
		}
		
		if(sub_dentry)
		{
			printk(KERN_EMERG"subdirs_dentry %s\n",sub_dentry->d_name.name);
			printk(KERN_EMERG"sub_dentry->d_inode->i_ino %d\n",sub_dentry->d_inode->i_ino);
		}

		if(sub_dentry->d_inode->i_ino == ino)
		{
			printk("9\n");
			return sub_dentry;
		}
	}
	return NULL;
}

//判断一个文件的所属权是不是本地的
int is_my_file(struct dentry *dentry)
{
	int i = 0, ans = 1;
	for(i = 0; i < 1024; i++)
	{
		if(reg_ino_mapp_table[i].used == 1)
		{
			if(reg_ino_mapp_table[i].local_ino == dentry->d_inode->i_ino)
			{
				ans = 0;
				break;
			}
		}
	}
	return ans;
}