#include <stdio.h>
#include <string.h>
#include <vfs.h>
#include <sys/printk.h>
#include <sys/fs.h>
#include <sys/tarfs.h>

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

struct dentry_reader *my_opendir(char *name)
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
struct file* my_open(char *pathname, int flags)
{
    int fs_type = get_fs_index(pathname);
    parse_path(pathname);
    printk("file name after parsing: %s\n", pathname);
    struct file* fd;
    //printk("fs type: %d\n", fs_type);

    if (fs_type == 0)
        fd = tarfs_open(pathname, flags);
    else if (fs_type == 1)
        fd = sata_open(pathname, flags);
    else
        fd = NULL;
    if (fd == NULL)
    {
        printk("Error opening file %s.\n", pathname);
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
