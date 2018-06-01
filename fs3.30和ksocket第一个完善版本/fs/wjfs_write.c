#include "wjfs_write.h"
#include "wjfs.h"

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

//服务端写入信息文件广播创建信息的函数,（目前是点对点的）
//ino 为远端写入信息文件的inode->i_ino
//name 为远端写入信息文件的名称
//ino_parent 为远端写入信息文件的父目录的 inode->i_ino
//name_parent 为远端写入信息文件的父目录的名称
//message 为要远程写入的信息
int wjfs_client_write(void *arg, char *ip, int port ,int ino, char *name,int ino_parent, char *name_parent, char *message)
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
