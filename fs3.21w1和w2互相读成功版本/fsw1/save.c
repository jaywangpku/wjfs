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

//在ln -s a f 测试中拿到了a 文件中的信息
void get_file_message()
{
	pgoff_t index = 0;
	unsigned long offset = 0;
	struct page *page;
	unsigned long nr, ret;
	pgoff_t end_index;
	loff_t isize;
	char *kaddr;
	unsigned long left;
	char __user buf[100];        //一定要记得开辟空间


	page = find_get_page(a_dentry->d_inode->i_mapping, index);
	if(page)
	{
		printk("page %d\n",page);
	}
	printk("a inode ino %d\n",a_dentry->d_inode->i_ino);
	isize = i_size_read(a_dentry->d_inode);
	printk("isize %d\n",isize);

	end_index = (isize - 1) >> PAGE_CACHE_SHIFT;
	nr = PAGE_CACHE_SIZE;
	if (index == end_index) {
		nr = ((isize - 1) & ~PAGE_CACHE_MASK) + 1;
	}
	nr = nr - offset;
	kaddr = kmap_atomic(page, KM_USER0);
	left = __copy_to_user_inatomic(buf, kaddr + offset, nr);
	kunmap_atomic(kaddr, KM_USER0);
	printk("wo la dao a de xinxi l\n");
	printk("a %s\n", buf);
}


//important function
page = find_get_page(a_dentry->d_inode->i_mapping, index);
isize = i_size_read(a_dentry->d_inode);



//不成功地寻找子目录项
struct qstr this;
	printk("5\n");
	char *name = "fff";
	unsigned long hash;
	unsigned int c;
	int len;
	printk("6\n");
	this.name = name;
	this.len = strlen(name);
	len = this.len;
	printk("3\n");
	hash = init_name_hash();
	printk("4\n");
	while (len--) {
		c = *(const unsigned char *)name++;
		if (c == '/' || c == '\0')
			return -EACCES;
		hash = partial_name_hash(c, hash);
	}
	printk("5\n");
	this.hash = end_name_hash(hash);
	printk("len %d\n",len);

	struct dentry *test_dentry = rroot_dentry;
	struct dentry *get_dentry;
	struct hlist_node *node;
	unsigned int hash1 = this.hash;
	printk("1\n");
	struct hlist_head *head = d_hash(test_dentry, hash1);
	printk("2\n");
	hlist_for_each_entry_rcu(get_dentry, node, head, d_hash)
	{
		printk("7\n");
		if(get_dentry && (get_dentry->d_parent == test_dentry))
		{
			printk("dentry_name %s\n", get_dentry->d_name.name);
		}
		else
		{
			printk("cuowu\n");
		}
	}


/*********************************************/
.h
#define D_HASHBITS     d_hash_shift
#define D_HASHMASK     d_hash_mask

static unsigned int d_hash_mask __read_mostly;
static unsigned int d_hash_shift __read_mostly;
static struct hlist_head *dentry_hashtable __read_mostly;
static __initdata unsigned long dhash_entries;
static int __init set_dhash_entries(char *str)
{
	if (!str)
		return 0;
	dhash_entries = simple_strtoul(str, &str, 0);
	return 1;
}
__setup("dhash_entries=", set_dhash_entries);

.c
static inline struct hlist_head *d_hash(struct dentry *parent, unsigned long hash)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	hash += ((unsigned long) parent ^ GOLDEN_RATIO_PRIME) / L1_CACHE_BYTES;
	hash = hash ^ ((hash ^ GOLDEN_RATIO_PRIME) >> D_HASHBITS);
	return dentry_hashtable + (hash & D_HASHMASK);
}

struct dentry * __wjfs_d_lookup(struct dentry * parent, struct qstr * name)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	unsigned int len = name->len;
	unsigned int hash = name->hash;
	const unsigned char *str = name->name;
	struct hlist_head *head = d_hash(parent,hash);
	struct dentry *found = NULL;
	struct hlist_node *node;
	struct dentry *dentry;
	printk("16\n");
	rcu_read_lock();
	if(head == NULL)
	{
		printk("head you mao bing\n");
	}
	if(d_hash == NULL)
	{
		printk("d_hash you nao bing\n");
	}
	
	hlist_for_each_entry_rcu(dentry, node, head, d_hash) {
		struct qstr *qstr;
		printk("17\n");
		if (dentry->d_parent == parent)
		{
			printk("dentry_name %s\n", dentry->d_name.name);
		}
// 		if (dentry->d_name.hash != hash)
// 			continue;
// 		if (dentry->d_parent != parent)
// 			continue;

// 		spin_lock(&dentry->d_lock);

// 		/*
// 		 * Recheck the dentry after taking the lock - d_move may have
// 		 * changed things.  Don't bother checking the hash because we're
// 		 * about to compare the whole name anyway.
// 		 */
// 		if (dentry->d_parent != parent)
// 			goto next;

// 		/* non-existing due to RCU? */
// 		if (d_unhashed(dentry))
// 			goto next;

		
// 		 * It is safe to compare names since d_move() cannot
// 		 * change the qstr (protected by d_lock).
		 
// 		qstr = &dentry->d_name;
// 		if (parent->d_op && parent->d_op->d_compare) {
// 			if (parent->d_op->d_compare(parent, qstr, name))
// 				goto next;
// 		} else {
// 			if (qstr->len != len)
// 				goto next;
// 			if (memcmp(qstr->name, str, len))
// 				goto next;
// 		}

// 		atomic_inc(&dentry->d_count);
// 		found = dentry;
// 		spin_unlock(&dentry->d_lock);
// 		break;
// next:
// 		spin_unlock(&dentry->d_lock);
 	}
 	rcu_read_unlock();
 	return found;
}


static struct dentry * wjfs_cached_lookup(struct dentry * parent, struct qstr * name, struct nameidata *nd)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct dentry * dentry = __wjfs_d_lookup(parent, name);

	/* lockess __d_lookup may fail due to concurrent d_move() 
	 * in some unrelated directory, so try with d_lookup
	 */
	// if (!dentry)
	// 	dentry = d_lookup(parent, name);

	// if (dentry && dentry->d_op && dentry->d_op->d_revalidate)
	// 	dentry = do_revalidate(dentry, nd);

	return dentry;
}

static struct dentry *__wjfs_lookup_hash(struct qstr *name,
		struct dentry *base, struct nameidata *nd)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct dentry *dentry;
	struct inode *inode;
	int err;

	inode = base->d_inode;
	printk("12\n");
	/*
	 * See if the low-level filesystem might want
	 * to use its own hash..
	 */
	if (base->d_op && base->d_op->d_hash) {
		printk("14\n");
		err = base->d_op->d_hash(base, name);
		printk("15\n");
		dentry = ERR_PTR(err);
		if (err < 0)
			goto out;
	}
	printk("13\n");
	//在dentry cache里面查找同名的 dentry结构 如果返回空，
	//这表示不存在同名的dentry结构，调用d_alloc创建一个新的dentry结构
	dentry = wjfs_cached_lookup(base, name, nd);  
	if (!dentry) {
		struct dentry *new;

		/* Don't create child dentry for a dead directory. */
		dentry = ERR_PTR(-ENOENT);
		if (IS_DEADDIR(inode))
			goto out;

		new = d_alloc(base, name);
		dentry = ERR_PTR(-ENOMEM);
		if (!new)
			goto out;
		dentry = inode->i_op->lookup(inode, new, nd);  //调用文件系统的lookup查找是否有同名的dentry
		if (!dentry)
			dentry = new;
		else
			dput(new);
	}
out:
	return dentry;
}

static int __wjfs_lookup_one_len(const char *name, struct qstr *this, struct dentry *base, int len)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	unsigned long hash;
	unsigned int c;
	printk("1\n");
	this->name = name;
	this->len = len;
	if (!len)
		return -EACCES;
	printk("2\n");
	hash = init_name_hash();
	printk("3\n");
	while (len--) {
		printk("4\n");
		c = *(const unsigned char *)name++;
		if (c == '/' || c == '\0')
			return -EACCES;
		printk("5\n");
		hash = partial_name_hash(c, hash);
	}
	printk("6\n");
	this->hash = end_name_hash(hash);
	printk("7\n");
	return 0;
}

struct dentry *wjfs_lookup_one_len(const char *name, struct dentry *base, int len)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	int err;
	struct qstr this;

	err = __wjfs_lookup_one_len(name, &this, base, len);
	printk("8\n");
	if (err)
		return ERR_PTR(err);
	printk("9\n");
	err = inode_permission(base->d_inode, MAY_EXEC);
	printk("10\n");
	if (err)
		return ERR_PTR(err);
	printk("11\n");
	return __wjfs_lookup_hash(&this, base, NULL);
}

test
int loop;
	dentry_hashtable =
		alloc_large_system_hash("Dentry cache",
					sizeof(struct hlist_head),
					dhash_entries,
					13,
					HASH_EARLY,
					&d_hash_shift,
					&d_hash_mask,
					0);

	for (loop = 0; loop < (1 << d_hash_shift); loop++)
		INIT_HLIST_HEAD(&dentry_hashtable[loop]);

	char *name = "ff1";
	wjfs_lookup_one_len(name, rroot_dentry, strlen(name));
	

//////////////////////////////////////////////////////////////////////////////////////////////////////
	printk("a dentry name : %s\n",a_dentry->d_name.name);
	struct dentry *a_parent_dentry = a_dentry->d_parent;
	printk(KERN_EMERG"根目录名称 %s\n",a_parent_dentry->d_name.name);

	struct list_head *plist;
	struct dentry *sub_dentry;

	list_for_each(plist, &(a_parent_dentry->d_subdirs))
	{
		sub_dentry = list_entry(plist, struct dentry, d_u.d_child);
		if(sub_dentry)
		{
			printk(KERN_EMERG"subdirs_dentry %s\n",sub_dentry->d_name.name);
			printk(KERN_EMERG"sub_dentry %ld\n",sub_dentry);
		}
	}
////////////////////////////////////////////////////////////////////////////////////////////////////
