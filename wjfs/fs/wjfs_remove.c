#include "wjfs_remove.h"
#include "wjfs.h"

//删除dir目录中的dentry目录项代表的文件   远端删除目录
int wjfs_rmdir_rdma(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	if (!simple_empty(dentry))
		return -ENOTEMPTY;
	drop_nlink(dentry->d_inode);
	simple_unlink(dir, dentry);
	drop_nlink(dir);
	return 0;
}

//从目录dir中删除dentry指定的索引节点对象   远端删除文件
int wjfs_unlink_rdma(struct inode *dir, struct dentry *dentry)
{
	printk(KERN_EMERG"wjfs: %s ==>> %s ==>> %d\n",__FILE__,__FUNCTION__,__LINE__);
	struct inode *inode = dentry->d_inode;
	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	drop_nlink(inode);
	dput(dentry);
	return 0;
}

//服务端删除文件广播创建信息的函数,（目前是点对点的）
//ino 为远端删除文件的inode->i_ino
//name 为远端删除文件的名称
//ino_parent 为远端删除文件的父目录的 inode->i_ino
//name_parent 为远端删除文件的父目录的名称
//mode 标记是文件还是文件夹
int wjfs_client_remove(void *arg, char *ip, int port ,int ino, char *name,int ino_parent, char *name_parent, int mode)
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