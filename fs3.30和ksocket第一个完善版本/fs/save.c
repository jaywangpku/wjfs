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

//在系统内部自动创建文件的函数
static int wjfs_create_by_name(const char *name, mode_t mode, struct dentry *parent, struct dentry **dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	int error = 0;
	if(!parent)
	{
		parent = rroot_dentry;
		//printk("父目录dentry错误\n");
		//return -EFAULT;
	}
	*dentry = NULL;
	*dentry = lookup_one_len(name, parent, strlen(name));

	int i = (int)atomic_read(&((*dentry)->d_count));
	printk("%s = %d\n",name, i);
	
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
	//在内核下创建完成之后需要将每一个dentry引用量减 1
	atomic_dec(&((*dentry)->d_count));
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
struct dentry_stat_t dentry_stat = {
	.age_limit = 45,
};

static void dentry_lru_del_init(struct dentry *dentry)
{
	if (likely(!list_empty(&dentry->d_lru))) {
		list_del_init(&dentry->d_lru);
		dentry->d_sb->s_nr_dentry_unused--;
		dentry_stat.nr_unused--;
	}
}

static struct kmem_cache *dentry_cache __read_mostly;

static void __d_free(struct dentry *dentry)
{
	WARN_ON(!list_empty(&dentry->d_alias));
	if (dname_external(dentry))
		kfree(dentry->d_name.name);
	kmem_cache_free(dentry_cache, dentry); 
}

static void d_callback(struct rcu_head *head)
{
	struct dentry * dentry = container_of(head, struct dentry, d_u.d_rcu);
	__d_free(dentry);
}

static void d_free(struct dentry *dentry)
{
	if (dentry->d_op && dentry->d_op->d_release)
		dentry->d_op->d_release(dentry);
	/* if dentry was never inserted into hash, immediate free is OK */
	if (hlist_unhashed(&dentry->d_hash))
		__d_free(dentry);
	else
		call_rcu(&dentry->d_u.d_rcu, d_callback);
}

static void wjfs_shrink_dcache_for_umount_subtree(struct dentry *dentry)
{
	struct dentry *parent;
	unsigned detached = 0;

	BUG_ON(!IS_ROOT(dentry));

	/* detach this root from the system */
	spin_lock(&dcache_lock);
	dentry_lru_del_init(dentry);
	__d_drop(dentry);
	spin_unlock(&dcache_lock);

	for (;;) {
		/* descend to the first leaf in the current subtree */
		while (!list_empty(&dentry->d_subdirs)) {
			struct dentry *loop;

			/* this is a branch with children - detach all of them
			 * from the system in one go */
			spin_lock(&dcache_lock);
			list_for_each_entry(loop, &dentry->d_subdirs,
					    d_u.d_child) {
				dentry_lru_del_init(loop);
				__d_drop(loop);
				cond_resched_lock(&dcache_lock);
			}
			spin_unlock(&dcache_lock);

			/* move to the first child */
			dentry = list_entry(dentry->d_subdirs.next,
					    struct dentry, d_u.d_child);
		}

		/* consume the dentries from this leaf up through its parents
		 * until we find one with children or run out altogether */
		do {
			struct inode *inode;

			if (atomic_read(&dentry->d_count) != 0) {
				printk(KERN_ERR
				       "BUG: Dentry %p{i=%lx,n=%s}"
				       " still in use (%d)"
				       " [unmount of %s %s]\n",
				       dentry,
				       dentry->d_inode ?
				       dentry->d_inode->i_ino : 0UL,
				       dentry->d_name.name,
				       atomic_read(&dentry->d_count),
				       dentry->d_sb->s_type->name,
				       dentry->d_sb->s_id);
				BUG();
			}

			if (IS_ROOT(dentry))
				parent = NULL;
			else {
				parent = dentry->d_parent;
				atomic_dec(&parent->d_count);
			}

			list_del(&dentry->d_u.d_child);
			detached++;

			inode = dentry->d_inode;
			if (inode) {
				dentry->d_inode = NULL;
				list_del_init(&dentry->d_alias);
				if (dentry->d_op && dentry->d_op->d_iput)
					dentry->d_op->d_iput(dentry, inode);
				else
					iput(inode);
			}

			d_free(dentry);

			/* finished when we fall off the top of the tree,
			 * otherwise we ascend to the parent and move to the
			 * next sibling if there is one */
			if (!parent)
				goto out;

			dentry = parent;

		} while (list_empty(&dentry->d_subdirs));

		dentry = list_entry(dentry->d_subdirs.next,
				    struct dentry, d_u.d_child);
	}
out:
	/* several dentries were freed, need to correct nr_dentry */
	spin_lock(&dcache_lock);
	dentry_stat.nr_dentry -= detached;
	spin_unlock(&dcache_lock);
}

void wjfs_shrink_dcache_for_umount(struct super_block *sb)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct dentry *dentry;

	if (down_read_trylock(&sb->s_umount))
		BUG();

	dentry = sb->s_root;
	sb->s_root = NULL;
	atomic_dec(&dentry->d_count);
	wjfs_shrink_dcache_for_umount_subtree(dentry);

	while (!hlist_empty(&sb->s_anon)) {
		dentry = hlist_entry(sb->s_anon.first, struct dentry, d_hash);
		wjfs_shrink_dcache_for_umount_subtree(dentry);
	}
}

LIST_HEAD(super_blocks);
DEFINE_SPINLOCK(sb_lock);

void wjfs_shutdown_super(struct super_block *sb)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	const struct super_operations *sop = sb->s_op;

	if (sb->s_root) {
		wjfs_shrink_dcache_for_umount(sb);
		sync_filesystem(sb);
		get_fs_excl();
		sb->s_flags &= ~MS_ACTIVE;

		/* bad name - it should be evict_inodes() */
		invalidate_inodes(sb);

		if (sop->put_super)
			sop->put_super(sb);

		/* Forget any remaining inodes */
		if (invalidate_inodes(sb)) {
			printk("VFS: Busy inodes after unmount of %s. "
			   "Self-destruct in 5 seconds.  Have a nice day...\n",
			   sb->s_id);
		}
		put_fs_excl();
	}
	spin_lock(&sb_lock);
	/* should be initialized for __put_super_and_need_restart() */
	list_del_init(&sb->s_list);
	list_del(&sb->s_instances);
	spin_unlock(&sb_lock);
	up_write(&sb->s_umount);
}

static DEFINE_IDA(unnamed_dev_ida);
static DEFINE_SPINLOCK(unnamed_dev_lock);/* protects the above */
static int unnamed_dev_start = 0; /* don't bother trying below it */

void wjfs_kill_anon_super(struct super_block *sb)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	int slot = MINOR(sb->s_dev);

	wjfs_shutdown_super(sb);
	spin_lock(&unnamed_dev_lock);
	ida_remove(&unnamed_dev_ida, slot);
	if (slot < unnamed_dev_start)
		unnamed_dev_start = slot;
	spin_unlock(&unnamed_dev_lock);
}

// 通过dentry向文件写入信息成功
static int wjfs_symlink(struct inode * dir, struct dentry *dentry, const char * symname)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	//test
	if(strcmp("server_read",dentry->d_name.name) == 0)
	{
		kthread_run(wjfs_server_read, NULL, "wjfs_server_read");
	}
	if(strcmp("server_create",dentry->d_name.name) == 0)
	{
		kthread_run(wjfs_server_create, NULL, "wjfs_server_create");
	}
	//在内核中将内容写入文件测试
	printk("1\n");
	char buf[1024] = "wo shi che shi yong de xinxi";
	struct iovec iov = { .iov_base = buf, .iov_len = strlen(buf) };
	struct kiocb kiocbb;
	struct file filep;
	struct file *filp = &filep;
	struct address_space *mapping = a_dentry->d_inode->i_mapping;
	struct inode *inode = a_dentry->d_inode;
	struct iov_iter i;
	struct page *page;
	pgoff_t index;
	unsigned long offset;
	unsigned long bytes;
	size_t copied;
	void *fsdata;
	loff_t pos = 0;
	char *kaddr;

	printk("2\n");
	init_sync_kiocb(&kiocbb, filp);
	kiocbb.ki_pos = 0;
	kiocbb.ki_left = strlen(buf);

	printk("3\n");
	iov_iter_init(&i, &iov, 1, 0, 0);

	printk("4\n");
	offset = (pos & (PAGE_CACHE_SIZE - 1));
	index = pos >> PAGE_CACHE_SHIFT;
	//bytes = min_t(unsigned long, PAGE_CACHE_SIZE - offset, iov_iter_count(&i));
	bytes = strlen(buf);

	printk("5\n");
	simple_write_begin(filp, mapping, pos, bytes, 0, &page, &fsdata);
	printk("6\n");
	//copied = iov_iter_copy_from_user_atomic(page, &i, offset, bytes);
	kaddr = kmap_atomic(page, KM_USER0);
	if (likely(i.nr_segs == 1)) 
	{
		printk("9\n");
		int left;
		char __user *buf = i.iov->iov_base + i.iov_offset;
		printk("buf: %s\n", buf);
		printk("bytes: %d\n", bytes);
		left = __copy_from_user_inatomic(kaddr + offset, buf, bytes);
		printk("left: %d\n", left);
		copied = bytes - left;
	}
	kunmap_atomic(kaddr, KM_USER0);

	printk("7\n");
	simple_write_end(filp, mapping, pos, bytes, copied, page, fsdata);
	printk("8\n");

	printk("wjfs_symlink over\n");
	return 0;
}

//对inode进行写的时候，是没法进行读操作的
ssize_t wjfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct iovec iov = { .iov_base = (void __user *)buf, .iov_len = len };  //把要写的信息填在这个buf中就好了
	struct kiocb kiocb;
	ssize_t ret;

	//在写之前，首先要判断信息要写到哪个地方去,先读取本地的内容，判断文件是否是远端的
	printk("filp->f_dentry : %s\n", filp->f_dentry->d_name.name);
	char message[1024];                                                    //本地文件中的内容
	char name[1024];                                                       //需要访问拿到信息的dentry名称
	memset(message, 0, sizeof(char)*1024);
	memset(name, 0, sizeof(char)*1024);
	loff_t i_size;
	unsigned int seq;  
	//在write里面调用，由于不是原子的，得不到正常值  在vfs_write中进行 rw_verify_area() 将要读取的部分给锁起来了
	//i_size = wjfs_i_size_read(filp->f_dentry->d_inode);    //对应inode中的信息数量 +1

	printk("isize: %d\n", i_size);
	//if(isize)                                                    //用数据才进行判断
	//{
		strcpy(name, filp->f_dentry->d_name.name);
		//get_file_message(name, message);
		// printk("wjfs_write get_message : %s\n", message);
	//}
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
	return ret;
}

static int wjfs_server_read(void *arg)
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
	addr_srv.sin_port = htons(server_port);
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
		kfree(tmp);

		memset(buf, 0, sizeof(char)*1024);
		memset(message, 0, sizeof(char)*1024);
		len = krecv(sockfd_cli, buf, sizeof(buf), 0);
		printk("w1 server got message : %s\n", buf);   //拿到的是要访问的文件的文件名

		if (memcmp(buf, "quit", 4) == 0)  //只比较前4个字节
			break;

		//拿到的信息就是要访问的文件的名称,扫描全部的dentry拿到inode
		get_file_message(buf, message);
		printk("message : %s\n",message);
		memset(buf, 0, sizeof(char)*1024);
		len = sprintf(buf, "%s", message);
		ksend(sockfd_cli, buf, len, 0);

		kclose(sockfd_cli);
	}
	
	kclose(sockfd_srv);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}

//进行远程读的客户端函数
//name为要远程访问的文件名
//message为远程访问得到的信息
static int wjfs_client_read(void *arg, char *ip, int port, char *name, char *message)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
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

	memset(buf, 0, sizeof(char)*1024);
	len = sprintf(buf, "%s", name);
	printk("要获得数据的文件名是: %s\n", buf);
	ksend(sockfd_cli, buf, len, 0);

	memset(buf, 0, sizeof(char)*1024);
	krecv(sockfd_cli, buf, 1024, 0);
	printk("client got message : %s\n", buf);

	strcpy(message, buf);

	kclose(sockfd_cli);

#ifdef KSOCKET_ADDR_SAFE
		set_fs(old_fs);
#endif

	return 0;
}