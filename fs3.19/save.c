/****************************************如此可以自行创建文件成功****************************************/
static int wjfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	// if(dir)
	// 	printk(KERN_EMERG"wjfs_symlink_dir:  %ld\n",dir->i_ino);
	// printk(KERN_EMERG"dentry->d_name.name:  %s\n",dentry->d_name.name);
	// if(dentry->d_inode)
	// 	printk(KERN_EMERG"dentry->d_inode.i_ino:  %ld\n",dentry->d_inode->i_ino);
	// if(symname)
	// 	printk(KERN_EMERG"symname:  %s\n",symname);
	//test
	printk("a dentry name : %s\n",a_dentry->d_name.name);
	struct dentry *a_parent_dentry = a_dentry->d_parent;
	printk(KERN_EMERG"根目录名称 %s\n",a_parent_dentry->d_name.name);

	struct list_head *plist;
	struct dentry *sub_dentry;
	
	
	list_for_each(plist, &(a_dentry->d_child))
	{
		sub_dentry = list_entry(plist, struct dentry, d_child);
		if(sub_dentry)
		{
			printk(KERN_EMERG"subdirs_dentry %d\n",sub_dentry->d_name.name);
			printk(KERN_EMERG"sub_dentry %ld\n",sub_dentry);
			//printk(KERN_EMERG"sub_dentry->d_name %ld\n",sub_dentry->d_name);
			//printk(KERN_EMERG"d_iname................. %s\n",sub_dentry->d_iname);
			//printk(KERN_EMERG"d_time................. %ld\n",sub_dentry->d_time);
		}
	}


	struct dentry *test_dentry;
	struct inode *test_inode;
	char *name = "wangjie";
	test_dentry = lookup_one_len(name, a_parent_dentry, strlen(name));
	test_inode = wjfs_get_inode(dir->i_sb, dir, S_IFREG | S_IRUGO, 0);
	d_instantiate(test_dentry, test_inode);
	dget(dentry);
	printk("test_dentry : %s\n",test_dentry->d_name.name);
	
	return 0;
}


/****************************************测试通信成功****************************************/
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


/****************************************目录项根据其内容查找子目录项失败****************************************/
	printk("a dentry name : %s\n",a_dentry->d_name.name);
	struct dentry *a_parent_dentry = a_dentry->d_parent;
	printk(KERN_EMERG"根目录名称 %s\n",a_parent_dentry->d_name.name);

	struct list_head *plist;
	struct dentry *sub_dentry;
	
	list_for_each(plist, &(a_dentry->d_subdirs))
	{
		sub_dentry = list_entry(plist, struct dentry, d_subdirs);
		if(sub_dentry)
		{
			printk(KERN_EMERG"subdirs_dentry %s\n",sub_dentry->d_name.name);
			printk(KERN_EMERG"sub_dentry %ld\n",sub_dentry);
			//printk(KERN_EMERG"sub_dentry->d_name %ld\n",sub_dentry->d_name);
			//printk(KERN_EMERG"d_iname................. %s\n",sub_dentry->d_iname);
			//printk(KERN_EMERG"d_time................. %ld\n",sub_dentry->d_time);
		}
	}


/***********************************************通过名称创建文件夹和文件成功************************************************/

static int wjfs_create_by_name(const char *name, mode_t mode, struct dentry *parent, struct dentry **dentry)
{
	int error = 0;
	if(!parent)
	{
		parent = rroot_dentry;
	}
	if(!parent)
	{
		printk("Ah! can not find a parent!\n");
		return -EFAULT;
	}
	*dentry = NULL;
	*dentry = lookup_one_len(name, parent, strlen(name));

	if(!IS_ERR(dentry))
	{
		if((mode & S_IFMT) == S_IFDIR)
			error = wjfs_mkdir(parent->d_inode, *dentry, mode);
		else
			error = wjfs_create(parent->d_inode, *dentry, mode, 0);
	}
	else
	{
		error = PTR_ERR(dentry);
	}
	return error;
}

struct dentry *wjfs_create_file(const char *name, mode_t mode, struct dentry *parent, void *data, struct file_operations *fops)
{
	struct dentry *dentry = NULL;
	int error;

	printk(KERN_EMERG"wjfs_creating file %s\n",name);
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
	return wjfs_create_file(name, S_IFDIR | S_IRWXU | S_IRUGO | S_IXUGO, parent, NULL, NULL);
}

void build_file(void)
{
	struct dentry *pslot;
	pslot = wjfs_create_dir("aa",NULL);
	wjfs_create_file("aa1", S_IFREG | S_IRUGO , pslot, NULL, NULL);
	wjfs_create_file("aa2", S_IFREG | S_IRUGO , pslot, NULL, NULL);
	wjfs_create_file("aa3", S_IFREG | S_IRUGO , pslot, NULL, NULL);
	pslot = wjfs_create_dir("bb",NULL);
	wjfs_create_file("bb1", S_IFREG | S_IRUGO , pslot, NULL, NULL);
	wjfs_create_file("bb2", S_IFREG | S_IRUGO , pslot, NULL, NULL);
	wjfs_create_file("bb3", S_IFREG | S_IRUGO , pslot, NULL, NULL);
}