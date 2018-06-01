#ifndef _WJFS_REMOVE_H
#define _WJFS_REMOVE_H

#include "wjfs.h"

//根据接收到的信息在本地删除一个文件
extern int rdma_remove(char *buf);

//本地删除文件时，发送消息让远端节点也删除一个文件
extern int wjfs_client_remove(void *arg, char *ip, int port ,int ino, char *name,
								int ino_parent, char *name_parent, int mode);

//利用这两个函数进行删除文件操作，但是这两个函数并不是删除文件的全部操作
extern int wjfs_rmdir_rdma(struct inode *dir, struct dentry *dentry);
extern int wjfs_unlink_rdma(struct inode *dir, struct dentry *dentry);

#endif