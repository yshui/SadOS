#include <stdio.h>
#include <string.h>
#include <vfs.h>
#include <sys/printk.h>
#include <sys/fs.h>
#include <sys/tarfs.h>

//check if current path is the root of a file system
int get_fs_index(char *str)
{
    if (strncmp(str, "/tarfs", 6) == 0)
        return 0;
    else if (strncmp(str, "/sata", 5) == 0)
        return 1;
    else if (strcmp(str, "/") == 0)
        return 2;
    //int i;
    //for (i = 0;i < super_block_count;++i)
    //    if (!strcmp(str, super_blocks[i].file_system_name) )
    //        return i;
    return -1;
}

void str_shift(char *str, int pos)
{
    int i;
    for (i = pos;str[i];++i)
        str[i - pos] = str[i];
    str[i - pos] = '\0';
}
void parse_path(char *name)
{
    if (!strncmp(name, "/tarfs", 6) )
        str_shift(name, 6);
    else if (!strncmp(name, "/sata", 5))
        str_shift(name, 5);
}

int my_close(struct file* fd)
{
    fd -> f_pos = 0;
    return 0;
}

ssize_t my_read(uint64_t fd_int, void *buf, size_t count)
{
    struct file* fd = (struct file*)fd_int;
    if (fd -> fs_type == 0)
        return tarfs_read(fd, buf, count);
    else if (fd -> fs_type == 1)
        return read_sata_wrapper(fd, buf, count);
    else return 0;
}
void my_write(uint64_t fd_int, const void *buf, size_t count)
{
    struct file* fd = (struct file*)fd_int;
    if (fd -> fs_type == 0)
    {
        printk("Error: Tar fs is read-only.\n");
    }
    else if (fd -> fs_type == 1)
        write_sata_wrapper(fd, buf, count);
    else
        printk("Cannot write file\n");
}

struct dentry_reader *my_open_dir(char *name)
{
    int fs_type = get_fs_index(name);
    struct dentry_reader *reader = NULL;
    //parser: remove FS name like "tarfs"
    parse_path(name);
    printk("path name after parsing: %s\n", name);
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
        printk("Invalid path.\n");

    if (reader)
        reader -> fs_type = fs_type;
    return reader;
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

struct file* my_open(char *pathname, int flags)
{
    int fs_type = get_fs_index(pathname);
    parse_path(pathname);
    struct file* fd;
    printk("file name after parsing: %s\n", pathname);

    if (fs_type == 0)
        fd = tarfs_open(pathname, flags);
    else if (fs_type == 1)
        fd = sata_open(pathname, flags);
    else
        fd = NULL;
    if (fd == NULL)
        printk("Error opening file %s.\n", pathname);
    return fd;
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
