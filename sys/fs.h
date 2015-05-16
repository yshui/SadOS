#pragma once
#include <vfs.h>
void init_fs(void);
struct file* sata_open(const char* pathname, int flags);
int read_sata_wrapper(struct file* fd, void *buf, int count);
int write_sata_wrapper(struct file* fd, const void *buf, int count);
struct dentry_reader* open_sata_dir(char *path);
struct dentry* read_sata_dir(struct dentry_reader *reader);
int close_sata_dir(struct dentry_reader *reader);
struct dentry_reader* open_root_dir(char *path);
struct dentry* read_root_dir(struct dentry_reader *reader);
void ls_sata_fs(char *path);
