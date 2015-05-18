/* Copyright (C) 2015, Yuxuan Shui <yshuiv7@gmail.com> */

/*
 * Everyone is allowed to copy and/or redistribute verbatim
 * copies of this code as long as the following conditions
 * are met:
 *
 * 1. The copyright infomation and license is unchanged.
 *
 * 2. This piece of code is not submitted as or as part of
 *    an assignment to the CSE506 course at Stony Brook
 *    University, unless the submitter is the copyright
 *    holder.
 */
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
