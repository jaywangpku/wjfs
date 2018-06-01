#ifndef _WJFS_WRITE_H
#define _WJFS_WRITE_H

#include "wjfs.h"

//根据收到的消息将收到的文件写入信息写入本地文件中
extern int rdma_write(char *buf);

//写信息时，如果是远端文件，这调用此客户函数将信息写入远端的文件中
extern int wjfs_client_write(void *arg, char *ip, int port ,int ino, char *name,
								int ino_parent, char *name_parent, char *message);

//普通文件file写操作函数  在此函数中，判断文件的类型，写入信息
extern ssize_t wjfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos);

#endif