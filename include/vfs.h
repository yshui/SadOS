#include <sys/defs.h>
struct super_block
{
    //struct list_head s_list;
    char file_system_name[12];
    int free_block_count;
    int free_inode_count;
    int block_size;
    int fs_size;
    struct dentry *s_root;
    int inode_begin;
    //struct list_head s_inodes;
    //struct list_head s_dirty;
    //struct list_head s_io;
    //struct list_head s_files;
};

int super_block_count;
struct super_block super_blocks[20];

struct timespec
{
    uint8_t year, month, day, hour, minute;
};

struct inode
{
    //struct timespec i_access_time;
    //struct timespec i_modify_time;
    //struct timespec i_create_time;
    int inode_id;
    int file_len;
    int block_count;
    int *i_data[15];
    uint16_t flags;
};

struct dentry
{
    struct super_block *d_sb;
    struct dentry *d_parent;
    struct dentry *d_subdirs[30];
    char d_iname[200];
    int dentry_id; //same as inode ID below
    struct inode *d_inode;
};
struct dentry_reader
{
    struct dentry *dentry;
    int offset;
};

struct file
{
    struct inode *inode;
    char fname[200];
    unsigned int f_flags;
    int f_mode;
    int f_pos;
};
