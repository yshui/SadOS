/* Copyright (C) 2015, Haochen Chen */

/*
 * Everyone is allowed to copy and/or redistribute verbatim
 * copies of this code as long as the following conditions
 * are met:
 *
 * 1. The copyright infomation and license is unchanged.
 *
 * 2. This piece of code is not submitted as or as part of
 *    an assignment to the CSE506 course at Stony Brook
 *    University, unless the copyright holder grant the
 *    submitter this permission.
 */
#include <stdio.h>
#include <string.h>
#include <vfs.h>
//#include <sys/printk.h>
#include <fs.h>
#include <tarfs.h>
#include <ipc.h>
#include <uapi/mem.h>
#include <uapi/ioreq.h>
#include <uapi/portio.h>
#include <bitops.h>
#include <errno.h>
#include <sendpage.h>
#include <sys/list.h>

char cwd[200] = "/";
char root[10] = "/", tarfs[200] = "/tarfs", sata[200] = "/sata";
void str_shift(char *str, int pos)
{
    int i;
    for (i = pos;str[i];++i)
        str[i - pos] = str[i];
    str[i - pos] = '\0';
}
//int strncmp(char *a, const char *b, int count)
//{
//    int i;
//    printk("str cmp: %s %s\n", a, b);
//    for (i = 0;i < count;++i)
//    {
//        if (a[i] < b[i])
//            return -1;
//        else if (a[i] > b[i])
//            return 1;
//    }
//    return 0;
//}
//there should be a working DIR for each process
char *my_getcwd(char *buf, size_t size)
{
    strcpy(buf, cwd);
    return buf;
}
void to_parent(char *path)
{
    int i, len = strlen(path);
    if (!strcmp(path, "/"))
        return;
    else
    {
        for (i = len - 1;i >= 0;--i)
            if (path[i] == '/')
                break;
        path[i] = '\0';
    }
}
int my_chdir(char* path)
{
    //printk("changing DIR: %s %s\n", path, cwd);
    if (strlen(path) == 0)
        return 0;
    if (!strcmp(path, "..") )
    {
        strcpy(path, cwd);
        int i, len = strlen(path);
        for (i = len - 1;i >= 0;--i)
            if (path[i] == '/')
                break;
        path[i] = '\0';
        strcpy(cwd, path);
    }
    //absolute path
    else if (path[0] == '/')
        strcpy(cwd, path);
    else
    {
        int len = strlen(cwd);
        if (cwd[len - 1] != '/')
            strcat(cwd, "/");
        strcat(cwd, path);
    }
    //printk("New current path: %s\n", cwd);
    return 0;
}

//check if current path is the root of a file system
int get_fs_index(char *str)
{
    if (strncmp(str, tarfs, 6) == 0)
        return 0;
    else if (strncmp(str, sata, 5) == 0)
        return 1;
    else if (strcmp(str, root) == 0)
        return 2;
    //int i;
    //for (i = 0;i < super_block_count;++i)
    //    if (!strcmp(str, super_blocks[i].file_system_name) )
    //        return i;
    return -1;
}

void parse_path(char *pathname)
{
    //printk("name:%s\n", pathname);
    if (!strncmp(pathname, tarfs, 6) )
    {
        //printk("IN TARFS!\n");
        str_shift(pathname, 6);
    }
    else if (!strncmp(pathname, sata, 5))
    {
        str_shift(pathname, 5);
    }
    if (strlen(pathname) == 0)
    {
        pathname[0] = '/';
        pathname[1] = '\0';
    }
}

int my_close(struct file* fd)
{
    fd -> f_pos = 0;
    return 0;
}

ssize_t my_read(uint64_t fd_int, void *buf, size_t count)
{
    struct file* fd = (struct file*)fd_int;
    //printk("FS TYPE: %d\n", fd->fs_type);
    if (fd -> fs_type == 0)
        return tarfs_read(fd, buf, count);
    else if (fd -> fs_type == 1)
        return read_sata_wrapper(fd, buf, count);
    else return 0;
}
ssize_t my_write(uint64_t fd_int, const void *buf, size_t count)
{
    struct file* fd = (struct file*)fd_int;
    if (fd -> fs_type == 0)
    {
        //printk("Error: Tar fs is read-only.\n");
	return -EBADF;
    }
    else if (fd -> fs_type == 1)
        return write_sata_wrapper(fd, buf, count);
    return -EINVAL;
}

struct dentry_reader *my_opendir(char *name)
{
    int fs_type = get_fs_index(name);
    struct dentry_reader *reader = NULL;
    //parser: remove FS name like "tarfs"
    parse_path(name);
    //printk("path name after parsing: %s\n", name);
    //TAR FS
    if (fs_type == 0)
        reader = open_tarfs_dir(name);
    //SATA FS
    else if (fs_type == 1)
        reader = open_sata_dir(name);
    //at root
    else if (fs_type == 2)
        reader = open_root_dir(name);
    else
        ;//printk("Invalid path.\n");

    if (reader)
        reader -> fs_type = fs_type;
    return reader;
}
struct file* my_open(char *pathname, int flags)
{
    int fs_type = get_fs_index(pathname);
    parse_path(pathname);
    //printk("file name after parsing: %s\n", pathname);
    struct file* fd;

    if (fs_type == 0)
        fd = tarfs_open(pathname, flags);
    else if (fs_type == 1)
        fd = sata_open(pathname, flags);
    else
        fd = NULL;
    if (fd == NULL)
    {
        //printk("Error opening file %s.\n", pathname);
        return NULL;
    }
    fd -> fs_type = fs_type;
    return fd;
}
struct dentry* my_readdir(struct dentry_reader* dentry_reader)
{
    //the only exception
    if (dentry_reader -> fs_type == 2)
        return read_root_dir(dentry_reader);
    else
    {
        if (dentry_reader -> fs_type == 0)
            return read_tarfs_dir(dentry_reader);
        else if (dentry_reader -> fs_type == 1)
            return read_sata_dir(dentry_reader);
    }
    return NULL;
}

void add_fs(const char* file_system_name, int super_block_count)
{
    strcpy(super_blocks[super_block_count].file_system_name, file_system_name);
    ++super_block_count;
}


//get current subdir
struct dentry* sata_get_dentry(struct dentry* cur_dentry, char *path_part)
{
    int i;
    struct dentry* tmp_dentry;

    //enumerate each subdir
    for (i = 0;cur_dentry -> d_subdirs[i] != NULL;++i)
    {
        tmp_dentry = cur_dentry -> d_subdirs[i];
        if (!strcmp(tmp_dentry -> d_iname, path_part) )
            return tmp_dentry;
    }
    return NULL;
}

off_t my_lseek(uint64_t fd_int, off_t offset, int whence)
{
    struct file* fd = (struct file*)fd_int;
    if (whence == SEEK_SET) 
        fd -> f_pos = offset;
    else if (whence == SEEK_CUR)
        fd -> f_pos += offset;
    else if (whence == SEEK_END)
        fd -> f_pos = fd -> inode -> file_len + offset;
    return fd -> f_pos;
}
//parsing the given path
//determine the file system, then determine the path
//we have 2 fs: /sata and /tarfs
//struct dentry* parse_path(char* path_str)
//{
//    int i, j, path_len = strlen(path_str);
//    char tmp_path[200] = {0}, cur_path_part[200] = {0};
//    int fs_index, max_len_fs = -1;
//    struct dentry *cur_dentry;
//    if (path_str[path_len - 1] != '/')
//        path_str[path_len++] = '/';
//
//    //file system
//    for (i = 0;i < path_len;++i)
//    {
//        tmp_path[i] = path_str[i];
//        if ( (fs_index = get_fs_index(tmp_path) ) != -1)
//            max_len_fs = i;
//    }
//    if (max_len_fs == -1)
//        return NULL;
//    cur_dentry = super_blocks[fs_index].s_root;
//    
//    for (i = max_len_fs + 1;i < path_len;++i)
//        tmp_path[i - max_len_fs - 1] = path_str[i];
//    tmp_path[i] = '\0';
//
//    for (i = 0;i < path_len;++i)
//    {
//        if (tmp_path[i] == '/')
//        {
//            memset(cur_path_part, 0, sizeof(cur_path_part) );
//            for (j = 0;j < i;++j)
//                cur_path_part[j] = tmp_path[j];
//            if ( (cur_dentry = sata_get_dentry(cur_dentry, cur_path_part) ) == NULL)
//                return NULL;
//            //prev = j + 1;
//        }
//    }
//    return cur_dentry;
//}

void strnncpy(char *a, char *b, int count)
{
    int i = 0;
    for (i = 0;i < count;++i)
        a[i] = b[i];
    a[i] = '\0';
}
struct list_head all_fds;
struct fd_list {
	int fd;
	struct list_node next;
};
uint64_t *handle_to_fd;

void close_handle (struct fd_list *fdi) {
	close(fdi->fd);
	handle_to_fd[fdi->fd] = 0;
	list_del(&fdi->next);
	free(fdi);
}
int portio_fd;
int main()
{
    portio_fd = port_connect(4, 0, NULL);
    init_fs();
    tarfs_init();
    long pd = open_port(6);
    struct fd_set fds;
    fds.nfds = 0;
    handle_to_fd = (uint64_t*) malloc(4096);
    memset(handle_to_fd, 0, 4096);
    uint64_t* handle_to_reader = (uint64_t*) malloc(4096);
    memset(handle_to_reader, 0, 4096);
    struct urequest ureq;
    list_head_init(&all_fds);
    //printf("In file system.\n");
    while (1)
    {
        fd_zero(&fds);
        fd_set_set(&fds, pd);
	struct fd_list *fdi;
	list_for_each(&all_fds, fdi, next)
            fd_set_set(&fds, fdi->fd);
        wait_on(&fds, NULL, 0);
        if (fd_is_set(&fds, pd)) {
            int handle = pop_request(pd, &ureq);
	    struct fd_list *fdn = malloc(sizeof(struct fd_list));
	    fdn->fd = handle;
	    list_add(&all_fds, &fdn->next);
        }
	struct fd_list *fdnxt;
	list_for_each_safe(&all_fds, fdi, fdnxt, next) {
            if (fd_is_set(&fds, fdi->fd)) {
                    int cookie = pop_request(fdi->fd, &ureq);
		    int handle = fdi->fd;
                    struct io_req *x = ureq.buf;
		    if (!x || ureq.len == 0) {
			    close_handle(fdi);
			    continue;
		    }
                    if (x -> type == IO_OPEN)
                    {
			struct io_res res;
			if (handle_to_fd[handle]) {
				res.err = EINVAL;
				respond(cookie, sizeof(res), &res);
				close_handle(fdi);
				continue;
			}
                        char *pathname = (char*) (x + 1);
                        struct file* cur_fd = my_open(pathname, x->flags);
                        //printf("cur fd: %d\n", cur_fd);
                        if (cur_fd == NULL) {
			    res.err = ENOENT;
			    respond(cookie, sizeof(res), &res);
			    close_handle(fdi);
			} else {
			    res.err = 0;
			    respond(cookie, sizeof(res), &res);
                handle_to_fd[handle] = (uint64_t)cur_fd;
			}
                    }
                    else if (x -> type == IO_OPENDIR)
                    {
                        char *pathname = (char *) (x + 1);
			struct io_res res;
			if (handle_to_fd[handle]) {
				res.err = EINVAL;
				respond(cookie, sizeof(res), &res);
				close_handle(fdi);
				continue;
			}
                        struct dentry_reader* cur_fd = my_opendir(pathname);
                        //printf("opening path: %s\n", pathname);
                        if (cur_fd != NULL) {
			    res.err = 0;
			    respond(cookie, sizeof(res), &res);
                            handle_to_fd[handle] = (uint64_t)cur_fd;
			} else {
				res.err = ENOENT;
				respond(cookie, sizeof(res), &res);
				close_handle(fdi);
			}
                    }
                    if (x -> type == IO_READ)
                    {
                        //printf("Reading file.\n");
                        uint64_t fd_int = handle_to_fd[handle];
                        struct io_res *rs = malloc(sizeof(struct io_res) + x->len + 1);
                        ssize_t ret = my_read(fd_int, rs+1, x->len);
			if (ret < 0) {
				rs->len = 0;
				rs->err = -ret;
			} else {
	                        rs -> len = ret;
        	                rs -> err = 0;
			}
                        respond(cookie, sizeof(struct io_res) + rs->len + 1, rs);
			free(rs);
                    }
                    else if (x -> type == IO_WRITE)
                    {
                        //printf("Writing file!\n");
                        uint64_t fd_int = handle_to_fd[handle];
                        void *buf = (void*) (x + 1);
                        ssize_t ret = my_write(fd_int, buf, x->len);
                        struct io_res rs;
			if (ret < 0) {
				rs.len = 0;
				rs.err = -ret;
			} else {
	                        rs.len = ret;
        	                rs.err = 0;
			}
                        respond(cookie, sizeof(struct io_res), &rs);
                    }
                    else if (x -> type == IO_READDIR)
                    { 
                        uint64_t fd = handle_to_fd[handle];
                        struct dentry_reader* dentry_reader = (struct dentry_reader*) fd;
                        struct dentry* ret = my_readdir(dentry_reader);
                        //printf("read dir: %x\n", ret);

                        struct io_res *rs = malloc(sizeof(struct io_res) + sizeof(struct dentry) + 1);
                        if (ret != NULL)
                        {
                            rs -> len = sizeof(struct dentry);
                            memcpy((char *)(rs + 1), (char*)ret, sizeof(struct dentry));
                        }
                        else
                            rs -> len = 0;
                        rs -> err = 0;
                        //printf("rs len: %d\n", rs->len);
                        respond(cookie, sizeof(struct io_res) + sizeof(struct dentry) + 1, rs);
                    }
                    else if (x -> type == IO_LSEEK)
                    {
                        uint64_t fd_int = handle_to_fd[handle];
                        off_t offset = x->len;
                        int whence = x->flags;
                        my_lseek(fd_int, offset, whence);
                        //off_t ret = my_lseek(fd_int, offset, whence); 
			struct io_res rs;
                        rs.len = 0;
                        rs.err = 0;
                        respond(cookie, sizeof(struct io_res), &rs);
                    }
            }
        }
    }
    return 0;
}
