#ifndef _WJFS_CREATE_H
#define _WJFS_CREATE_H

#include "wjfs.h"

//在系统内部自动创建文件的函数
extern int wjfs_create_by_name(const char *name, mode_t mode, struct dentry *parent, struct dentry **dentry);
extern struct dentry *wjfs_create_dir(const char *name, struct dentry *parent);
extern struct dentry *wjfs_create_file(const char *name, mode_t mode, struct dentry *parent, 
										void *data, struct file_operations *fops);

//根据接收到的信息在本地创建一个文件
extern int rdma_create(char *buf, char *ip);

//本地创建文件时，发送消息让远端节点也创建一个文件
extern int wjfs_client_create(void *arg, char *ip, int port ,int ino, char *name,
								int ino_parent, char *name_parent, int mode);

//本地接收到远端创建文件的信息时，调用此函数进行创建文件，避免循环创建
extern int wjfs_mknod_rdma(struct inode *dir, struct dentry *dentry, int mode, dev_t dev);

#endif