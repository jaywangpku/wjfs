#include "wjfs.h"

struct super_block *rroot_sb;       //指向super_block的指针
struct dentry *rroot_dentry;        //
struct inode *rroot_inode;
struct dentry *a_dentry;

static struct backing_dev_info wjfs_backing_dev_info = {
	.name		= "wjfs",
	.ra_pages	= 0,	/* No readahead */
	.capabilities	= BDI_CAP_NO_ACCT_AND_WRITEBACK |
			  BDI_CAP_MAP_DIRECT | BDI_CAP_MAP_COPY |
			  BDI_CAP_READ_MAP | BDI_CAP_WRITE_MAP | BDI_CAP_EXEC_MAP,
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

//what dir used for ??
//create a new special dev file, file in dir and it's dentry in dentry
static int wjfs_mknod(struct inode *dir, struct dentry *dentry, int mode, dev_t dev)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
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

//删除dir目录中的dentry目录项代表的文件
static int wjfs_rmdir(struct inode *dir, struct dentry *dentry)
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
	void *arg;
	if(strcmp("server",dentry->d_name.name) == 0)
	{
		kthread_run(wjfs_server, NULL, "wjfs_server");
		//wjfs_server(arg);
	}
	if(strcmp("client",dentry->d_name.name) == 0)
	{
		kthread_run(wjfs_client, NULL, "wjfs_client");
		//wjfs_client(arg);
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

//从目录dir中删除dentry指定的索引节点对象
static int wjfs_unlink(struct inode *dir, struct dentry *dentry)
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

	//还没有实现但是需要用到的函数
	//.lookup       = simple_lookup,
	//.link		    = simple_link,
	//.unlink		= simple_unlink,
	//.rename		= simple_rename,
	//.rmdir		= simple_rmdir,
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
	
	//data 输出为null
	//printk("data : %s\n",data);
	
	struct wjfs_fs_info *fsi;
	struct inode *root_inode;
	struct dentry *root_dentry;
	int err = 0;

	//If filesystem uses generic_show_options() in wjfs_ops , this function should be called from the fill_super() callback.
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

	// ??
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
	return 0;

	// //通过全局变量将三个导出来方便使用
	// rroot_sb = sb;
	// rroot_dentry = sb->s_root;
	// rroot_inode = sb->s_root->d_inode;

	// // look for the info of root inode and dentry
	// struct qstr root_name_dentry;
	// root_name_dentry = sb->s_root->d_name;
	
	// //dentry info
	// printk(KERN_EMERG"root_name_dentry %s",root_name_dentry.name);
	// printk(KERN_EMERG"root_parent_name_dentry %s",sb->s_root->d_parent->d_name.name);
	// //inode info
	// printk(KERN_EMERG"root_inode_num %ld",root_inode->i_ino);

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

void wjfs_kill_sb(struct super_block *sb)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	/* 卸载一个文件系统时要做的操作 */
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
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

const struct file_operations wjfs_file_operations = {
	.read		= do_sync_read,
	.aio_read	= generic_file_aio_read,
	.write		= do_sync_write,
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
	addr_srv.sin_addr.s_addr = inet_addr("192.168.124.133");
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

	krecv(sockfd_cli, buf, 1024, 0);
	printk("client got message : %s\n", buf);

	len = sprintf(buf, "%s", "Hello, I am client\n");
	ksend(sockfd_cli, buf, len, 0);

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
	printk("server got connected from : %s %d\n", tmp, ntohs(addr_cli.sin_port));
	kfree(tmp);

	len = sprintf(buf, "%s", "Hello, welcome to ksocket tcp srv service\n");
	ksend(sockfd_cli, buf, len, 0);

	memset(buf, 0, sizeof(buf));
	len = krecv(sockfd_cli, buf, sizeof(buf), 0);
	printk("server got message : %s\n", buf);

	len = sprintf(buf, "%s", "I am server, I want to close\n");
	ksend(sockfd_cli, buf, len, 0);

	kclose(sockfd_cli);
	kclose(sockfd_srv);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}




