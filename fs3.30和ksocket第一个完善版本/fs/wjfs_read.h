#ifndef _WJFS_READ_H
#define _WJFS_READ_H

#include "wjfs.h"

//根据收到的消息读取本地文件中的信息
extern int rdma_read(char *buf);

//读取信息时，如果是远端文件，这调用此客户函数读取远端文件的信息
extern int wjfs_client_read(void *arg, char *ip, int port ,int ino, char *name,
								int ino_parent, char *name_parent, char *message);

//普通文件file读操作函数   在此函数中，判断文件的类型，读取信息
extern ssize_t wjfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos);

#endif