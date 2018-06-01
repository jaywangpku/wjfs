#include "wjfs_create.h"
#include "wjfs.h"

// 远端创建文件使用的创建函数
int wjfs_mknod_rdma(struct inode *dir, struct dentry *dentry, int mode, dev_t dev)
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

//服务端创建文件广播创建信息的函数,（目前是点对点的）
//ino 为远端创建文件的inode->i_ino
//name 为远端创建文件的名称
//ino_parent 为远端创建文件的父目录的 inode->i_ino
//name_parent 为远端创建文件的父目录的名称
//mode 标记是文件还是文件夹
int wjfs_client_create(void *arg, char *ip, int port ,int ino, char *name,int ino_parent, char *name_parent, int mode)
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

//在系统内部自动创建文件的函数
int wjfs_create_by_name(const char *name, mode_t mode, struct dentry *parent, struct dentry **dentry)
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