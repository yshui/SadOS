#include <sys/tarfs.h>
#include <sys/printk.h>
#include <string.h>
#include <vfs.h>
#define TAR_COUNT   200
extern char cwd[200];

struct file temp_fd;
struct dentry_reader dentry_reader;
struct dentry tarfs_entry[TAR_COUNT];
struct inode tarfs_inode[TAR_COUNT];
int tar_file_count = 0;
int oct_to_int(char *oct_str)
{
    int i, ret_value = 0;
    
    for (i = 0;oct_str[i];++i)
        ret_value = 8 * ret_value + oct_str[i] - '0';
    return ret_value;
}

//return -1 when file doesn't exists
int get_file_index(const char* fname)
{
    int i;
    for (i = 0;i < tar_file_count;++i)
        if (!strcmp(tarfs_entry[i].d_iname, fname) )
            return i;
    return -1;
}

int tarfs_close(struct file* fd)
{
    fd -> f_pos = 0;
    return 0;
}
int tarfs_read(struct file *fd, void *buf, int count)
{
    int i;
    char *buf_ptr = (char *)buf;
    //should skip the first block, which is the index
    //reach the end
    if (fd -> f_pos == fd -> inode -> file_len)
        return 0;
    if (fd -> f_pos + count >= fd -> inode -> file_len)
        count = fd -> inode -> file_len - fd -> f_pos;
    char* cur_loc = (char *)fd -> inode -> i_data[0];
    for (i = 0;i < count;++i)
        buf_ptr[i] = cur_loc[i + fd -> f_pos];
    buf_ptr[i] = '\0';
    fd -> f_pos += count;
    return count;
}
struct file* tarfs_open(const char* pathname, int flags)
{
    int file_index;
    //struct file* fd = (struct file*) malloc(sizeof(struct file) );
    struct file *fd = &temp_fd;

    if ( (file_index = get_file_index(pathname) ) == -1)
    {
        printk("File doesn't exist.\n");
        return NULL;
    }
    strcpy(fd -> fname, pathname);
    fd -> inode = &tarfs_inode[file_index];
    fd -> f_pos = 0;
    return fd;
}

struct dentry_reader* open_tarfs_dir(char *dir_path)
{
    int i;
    for (i = 0;i < tar_file_count;++i)
        if (!strcmp(tarfs_entry[i].d_iname, dir_path))
        {
            dentry_reader.dentry = &tarfs_entry[i];
            dentry_reader.offset = 0;
            return &dentry_reader;
        }
    return NULL;
}
//return current subdir
struct dentry* read_tarfs_dir(struct dentry_reader* reader)
{
    if (reader -> dentry -> d_subdirs[reader -> offset] == NULL)
        return NULL;
    ++reader -> offset;
    return reader -> dentry -> d_subdirs[reader -> offset - 1];
}
int close_tarfs_dir(struct dentry_reader* reader)
{
    reader -> offset = 0;
    return 0;
}

void tarfs_node_init(struct dentry* cur_dentry, struct super_block *d_sb, struct dentry *d_parent, char *d_iname, struct inode *d_inode)
{
    int i;
    cur_dentry -> d_sb = d_sb;
    cur_dentry -> d_parent = d_parent;
    for (i = 0;i < 30;++i)
        cur_dentry -> d_subdirs[i] = NULL;
    strcpy(cur_dentry -> d_iname, d_iname);
    if (d_parent != NULL)
        for (i = 0;i < 30;++i)
            if (d_parent -> d_subdirs[i] == NULL)
            {
                //printk("%s %d %s\n", d_parent -> d_iname, i, cur_dentry -> d_iname);
                d_parent -> d_subdirs[i] = cur_dentry;
                break;
            }
    cur_dentry -> d_inode = d_inode;
}

//find parent for cur dentry
struct dentry* get_parent(char *cur_dentry_name)
{
    int i = 0, j, len = strlen(cur_dentry_name);
    char parent_dir[200] = {0};
    for (i = len - 1;i >= 0;--i)
        if (cur_dentry_name[i] == '/')
        {
            if (i == 0)
                parent_dir[0] = '/';
            else
            {
                for (j = 0;j < i;++j)
                    parent_dir[j] = cur_dentry_name[j];
            }
            break;
        }
    //printk("parent dir: %s\n", parent_dir);

    for (i = 0;i < tar_file_count;++i)
    {
        //is parent
        if (!strcmp(tarfs_entry[i].d_iname, parent_dir) )
            return &tarfs_entry[i];
    }
    return NULL;
}

void change_format(char ch, char *str)
{
    int i, len = strlen(str);
    for (i = len - 1;i >= 0;--i)
        str[i + 1] = str[i];
    str[0] = ch;
    if (str[len] == '/')
        str[len] = '\0';
    else
        str[len + 1] = '\0';
}
void tarfs_init()
{
    //char tarfs_name[10] = "/tarfs";
    //strcpy(super_blocks[super_block_count].file_system_name, "tarfs");
    //super_blocks[super_block_count].free_block_count = 10000;
    //super_blocks[super_block_count].block_size = 512;
    //super_blocks[super_block_count].fs_size = (struct posix_header_ustar*) (&_binary_tarfs_end) - (struct posix_header_ustar*) (&_binary_tarfs_start);
    //super_blocks[super_block_count].s_root = (&tarfs_entry[0]);
    //root node in tarfs
    tarfs_inode[0].block_count = 1;
    tarfs_inode[0].file_len = 0;
    tarfs_inode[0].i_data[0] = 0;
    tarfs_node_init(&tarfs_entry[0], &super_blocks[super_block_count - 1], NULL, "/", &tarfs_inode[0]);
    //super_block_count++;
    //the only node: root in tarfs
    tar_file_count = 1;

    int cur_file_size = 0, cur_block_count = 0;
    struct posix_header_ustar *header = (struct posix_header_ustar*) (&_binary_tarfs_start);

    while (header != (struct posix_header_ustar*)(&_binary_tarfs_end) )
    {
        if (strlen(header -> name) == 0)
            break;
        //printk("%s ",header -> name );
        //printk("%c\n", header -> typeflag[0]);
        change_format('/', header -> name);
        cur_file_size = oct_to_int(header -> size);
        //printk("%d\n", cur_file_size);
        if (cur_file_size % 512 == 0)
            cur_block_count = cur_file_size / 512;
        else
            cur_block_count = cur_file_size / 512 + 1;
        //build inode
        tarfs_inode[tar_file_count].block_count = cur_block_count;
        tarfs_inode[tar_file_count].file_len = cur_file_size;
        //data starting pointer
        tarfs_inode[tar_file_count].i_data[0] = (int *)(header + 1);
        //build dentry
        struct dentry* cur_par = get_parent(header -> name);
        //printk("%s\n", header -> name);
        //printk("%s\n", cur_par -> d_iname);
        tarfs_node_init(&tarfs_entry[tar_file_count], &super_blocks[super_block_count - 1], cur_par, header -> name, &tarfs_inode[tar_file_count]);

        header += cur_block_count;
        header++;
        tar_file_count++;
    }
}

void ls_tarfs(char *path)
{
    struct dentry_reader* reader = open_tarfs_dir(path);
    struct dentry * cur_dentry;
    while ( (cur_dentry = read_tarfs_dir(reader) ) != NULL)
    {
        printk("%s\n", cur_dentry -> d_iname);
    }
    close_tarfs_dir(reader);
}
