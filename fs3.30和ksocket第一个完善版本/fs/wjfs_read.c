#include "wjfs_read.h"
#include "wjfs.h"

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

//服务端读取信息文件广播创建信息的函数,（目前是点对点的）
//ino 为远端读取信息文件的inode->i_ino
//name 为远端读取信息文件的名称
//ino_parent 为远端读取信息文件的父目录的 inode->i_ino
//name_parent 为远端读取信息文件的父目录的名称
//message 为要远程读取的信息
int wjfs_client_read(void *arg, char *ip, int port ,int ino, char *name,int ino_parent, char *name_parent, char *message)
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