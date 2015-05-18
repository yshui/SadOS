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
 *    University, unless the copyrighter grant the permssion
 *    to the submitter.
 */
#include <vfs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <sys/mm.h>
//#include <sys/printk.h>
#include <sendpage.h>
#include <uapi/port.h>
#include <uapi/mem.h>
#include <ipc.h>
#include <errno.h>
#include "ahci.h"
#define BLOCK_SIZE  512
#define FS_SIZE     (BLOCK_SIZE * 64)
#define INODE_BITMAP_START_INDEX    1
#define DATA_BITMAP_START_INDEX     2
#define DENTRY_BITMAP_START_INDEX   3
#define INODE_START_INDEX   4
#define DENTRY_START_INDEX  14
#define DATA_START_INDEX    34
#define INODE_COUNT (10 * 4)
#define DATA_BLOCK_COUNT    10
#define TOTAL_BLOCK_COUNT (4 + 10 + 20 + DATA_BLOCK_COUNT)

struct file temp_fd;
struct super_block cur_super_block;
//struct dentry cur_dentry;
//struct dentry_reader cur_dentry_reader;
HBA_MEM* abar;
char fs[FS_SIZE];
//a common buffer in kernel space, used for read/write file
char *common_buf;
char inode_bitmap[BLOCK_SIZE + 2], dentry_bitmap[BLOCK_SIZE + 2], data_bitmap[BLOCK_SIZE + 2];
struct port_data **pdtable;
//given index, get block from hard disk
char *get_block(int index)
{
    //for hard disk, return a block of data
    read_sata(pdtable[0], index + DATA_START_INDEX, 0, 1, common_buf);
    return common_buf;
}

void read_inode_bitmap(char *inode_bitmap)
{
    read_sata(pdtable[0], INODE_BITMAP_START_INDEX, 0, 1, common_buf);
    memcpy(inode_bitmap, common_buf, BLOCK_SIZE);
}
void read_data_bitmap(char *data_bitmap)
{
    read_sata(pdtable[0], DATA_BITMAP_START_INDEX, 0, 1, common_buf);
    memcpy(data_bitmap, common_buf, BLOCK_SIZE);
}
void read_dentry_bitmap(char *dentry_bitmap)
{
    read_sata(pdtable[0], DENTRY_BITMAP_START_INDEX, 0, 1, common_buf);
    memcpy(dentry_bitmap, common_buf, BLOCK_SIZE);
}

uint64_t get_free_inode(void)
{
    uint64_t i;

    read_inode_bitmap(inode_bitmap);
    for (i = 0;i < BLOCK_SIZE;++i)
        if (inode_bitmap[i] == 0)
            return i;
    return -1;
}
uint64_t get_free_dentry(void)
{
    uint64_t i;

    read_dentry_bitmap(dentry_bitmap);
    for (i = 0;i < BLOCK_SIZE;++i)
        if (dentry_bitmap[i] == 0)
            return i;
    return -1;
}

uint64_t get_free_data(void)
{
    uint64_t i;

    read_data_bitmap(data_bitmap);
    for (i = 0;i < BLOCK_SIZE;++i)
        if (data_bitmap[i] == 0)
            return i;
    return -1;
}

void set_inode_bitmap(int inode_id)
{
    read_inode_bitmap(inode_bitmap);
    inode_bitmap[inode_id] = 1;
    memcpy(common_buf, inode_bitmap, BLOCK_SIZE);
    write_sata(pdtable[0], INODE_BITMAP_START_INDEX, 0, 1, common_buf);
}
void set_data_bitmap(int data_id)
{
    read_data_bitmap(data_bitmap);
    data_bitmap[data_id] = 1;
    memcpy(common_buf, data_bitmap, BLOCK_SIZE);
    write_sata(pdtable[0], DATA_BITMAP_START_INDEX, 0, 1, common_buf);
}
void set_dentry_bitmap(int dentry_id)
{
    read_dentry_bitmap(dentry_bitmap);
    dentry_bitmap[dentry_id] = 1;
    memcpy(common_buf, dentry_bitmap, BLOCK_SIZE);
    write_sata(pdtable[0], DENTRY_BITMAP_START_INDEX, 0, 1, common_buf);
}

void build_inode(int inode_id, uint16_t flags)
{
    //set bitmap
    set_inode_bitmap(inode_id);
    int inode_block_id = inode_id / 4, inode_block_offset = inode_id % 4;
    read_sata(pdtable[0], INODE_START_INDEX + inode_block_id, 0, 1, common_buf);
    //struct inode *new_inode = (struct inode*) (fs + INODE_START_ADDR + inode_id * sizeof(struct inode));
    struct inode *new_inode = (struct inode*) common_buf;
    new_inode += inode_block_offset;
    new_inode -> inode_id = inode_id;
    new_inode -> file_len = 0;
    new_inode -> block_count = 0;
    int i;
    //0: NULL block
    //deprecated
    for (i = 0;i < 13;++i)
        new_inode -> i_data[i] = 0;
    new_inode -> flags = flags;
    write_sata(pdtable[0], INODE_START_INDEX + inode_block_id, 0, 1, common_buf);
}

void write_dentry(int inode_index, struct dentry* dentry_block)
{
    int dentry_block_id = inode_index / 2, dentry_block_offset = inode_index % 2;
    char *dentry_block_ptr = (char *)dentry_block;
    read_sata(pdtable[0], DENTRY_START_INDEX + dentry_block_id, 0, 1, common_buf);
    struct dentry* common_buf_ptr = (struct dentry*)common_buf;
    common_buf_ptr += dentry_block_offset;
    memcpy((char *)common_buf_ptr, dentry_block_ptr, sizeof(struct dentry) );
    write_sata(pdtable[0], DENTRY_START_INDEX + dentry_block_id, 0, 1, common_buf);
}
void build_dentry(uint64_t inode_id, uint64_t dentry_id, uint64_t d_parent, const char* d_iname)
{
    //set bitmap
    set_dentry_bitmap(dentry_id);
    int dentry_block_id = dentry_id / 2, dentry_block_offset = dentry_id % 2;
    read_sata(pdtable[0], DENTRY_START_INDEX + dentry_block_id, 0, 1, common_buf);
    struct dentry *new_dentry = (struct dentry*) malloc(sizeof(struct dentry));
    new_dentry += dentry_block_offset;
    new_dentry -> d_parent = (struct dentry*) d_parent;
    int i;
    for (i = 0;i < 24;++i)
        new_dentry -> d_subdirs[i] = 0;
    strcpy(new_dentry -> d_iname, d_iname);
    //actually stores inode ID
    new_dentry -> d_inode = (struct inode*) inode_id;
    new_dentry -> dentry_id = dentry_id;
    //printk("dentry id: %d\n", dentry_id);
    //printk("inode id: %d\n", inode_id);
    //printk("common buf for dentry1: %s\n", new_dentry -> d_iname);
    //printf("common buf for dentry2: %s\n", new_dentry -> d_iname);

    write_dentry(dentry_id, new_dentry);
    //write_sata(pdtable[0], DENTRY_START_INDEX + dentry_block_id, 0, 1, (char*) new_dentry);
}
void get_dentry(int inode_index, struct dentry* dentry_block)
{
    int dentry_block_id = inode_index / 2, dentry_block_offset = inode_index % 2;
    char *dentry_block_ptr = (char *)dentry_block;
    read_sata(pdtable[0], DENTRY_START_INDEX + dentry_block_id, 0, 1, common_buf);
    struct dentry* common_buf_ptr = (struct dentry*)common_buf;
    common_buf_ptr += dentry_block_offset;
    memcpy(dentry_block_ptr, (char *)common_buf_ptr, sizeof(struct dentry) );
}

void write_inode(int inode_index, struct inode* inode_block)
{
    int inode_block_id = inode_index / 4, inode_block_offset = inode_index % 4;
    read_sata(pdtable[0], INODE_START_INDEX + inode_block_id, 0, 1, common_buf);
    struct inode* common_buf_ptr = (struct inode*) common_buf;
    common_buf_ptr += inode_block_offset;
    memcpy( (char *)common_buf_ptr, (char *)inode_block, sizeof(struct inode) );
    write_sata(pdtable[0], INODE_START_INDEX + inode_block_id, 0, 1, common_buf);
}

void get_inode(int inode_index, struct inode* inode_block)
{
    int inode_block_id = inode_index / 4, inode_block_offset = inode_index % 4;
    read_sata(pdtable[0], INODE_START_INDEX + inode_block_id, 0, 1, common_buf);
    struct inode* common_buf_ptr = (struct inode*) common_buf;
    common_buf_ptr += inode_block_offset;
    memcpy((char *)inode_block, (char *)common_buf_ptr, sizeof(struct inode) );
}

void get_data(uint64_t data_index, char *data_block, int offset, int count)
{
    read_sata(pdtable[0], DATA_START_INDEX + data_index, 0, 1, common_buf);
    //printk("Reading common buf: %s %d\n", common_buf, offset);
    memcpy(data_block, common_buf + offset, count);
}
void write_data(int data_index, char *data, int offset, int count)
{
    read_sata(pdtable[0], DATA_START_INDEX + data_index, 0, 1, common_buf);
    memcpy(common_buf + offset, data, count);
    //printk("Writing common buf: %s %d\n", common_buf + offset, offset);
    write_sata(pdtable[0], DATA_START_INDEX + data_index, 0, 1, common_buf);
}

int sata_get_parent(const char *cur_dentry_name)
{
    //printk("get parent: %s\n", cur_dentry_name)
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
    //printf("Parent dir: %s\n", parent_dir);

    read_inode_bitmap(inode_bitmap);
    struct dentry* cur_dentry = (struct dentry*) malloc(sizeof(struct dentry));
    for (i = 0;i < BLOCK_SIZE;++i)
    {
        if (inode_bitmap[i] == 0)
            continue;
        get_dentry(i, cur_dentry);
        //printf("dentry id: %d\n", i);
        //printf("?????cur dentry: %s\n", cur_dentry ->d_iname);
        if (!strcmp(cur_dentry ->d_iname, parent_dir) )
        {
            //printk("parent dentry id: %d %s %s\n", cur_dentry.dentry_id, cur_dentry.d_iname, parent_dir);
            return cur_dentry ->dentry_id;
        }
    }
    return -1;
}

struct file* sata_open(const char* pathname, int flags)
{
    //printk("current path name: %s\n", pathname);
    uint64_t i;
    char inode_bitmap[BLOCK_SIZE];
    //struct file *fd = (struct file *)get_page();
    struct file* fd = (struct file*) malloc(sizeof(struct file));
    fd -> fs_type = 1;
    struct inode cur_inode;
    struct dentry* c_dentry = (struct dentry*)malloc(sizeof(struct dentry));
    struct dentry* parent_dentry = (struct dentry*)malloc(sizeof(struct dentry));
    strcpy(fd -> fname, pathname);
    fd -> f_flags = flags;
    fd -> f_pos = 0;

    read_inode_bitmap(inode_bitmap);
    for (i = 0;i < BLOCK_SIZE;++i)
    {
        if (inode_bitmap[i] == 0)
            continue;
        get_inode(i, &cur_inode);
        get_dentry(i, c_dentry);
        //printk("current dentry: %s\n", cur_dentry.d_iname);
        //printk("current dentry id: %d\n", i);

        if (!strcmp(c_dentry -> d_iname, pathname) )
        {
            fd -> inode = (struct inode *)i;
            return fd;
        }
    }
    if (flags == O_CREAT)
    {
        int parent_dentry_id = sata_get_parent(pathname);
        if (parent_dentry_id == -1)
        {
            printf("Cannot find the file path.\n");
            return NULL;
        }
        uint64_t inode_id = get_free_inode();
        //printk("free inode id: %d\n", inode_id);
        uint64_t dentry_id = get_free_dentry();
        if (inode_id == -1 || dentry_id == -1)
        {
            printf("Disk is full.\n");
            return NULL;
        }
        fd -> inode = (struct inode *)inode_id;
        build_inode(inode_id, flags);
        build_dentry(inode_id, dentry_id, parent_dentry_id, pathname);
        //add subdir in parent dentry
        get_dentry(parent_dentry_id, parent_dentry);
        for (i = 0;i < 24;++i)
            if (parent_dentry -> d_subdirs[i] == NULL)
            {
                parent_dentry -> d_subdirs[i] = (struct dentry*) inode_id;
                break;
            }
        write_dentry(parent_dentry_id, parent_dentry);
        //printf("Create inode successfully.\n");
        return fd;
    }
    //file does not exist
    else
        printf("Error: file doesn't exist.\n");
    return NULL;
}
int close_sata(struct file* fd)
{
    fd -> f_pos = 0;
    free(fd);
    return 0;
}
//read blocks from SATA
int read_sata_wrapper(struct file* fd, void *buf, int count)
{
    //printk("IN READ SATA WRAPPER.\n");
    int i;
    char *buf_ptr = (char *)buf;
    struct inode cur_inode_instance;
    struct inode* cur_inode = &cur_inode_instance;
    get_inode((uint64_t) fd -> inode, cur_inode);
    //printk("inode index: %d\n", (uint64_t)fd -> inode);
    //printk("f_pos & count: %d %d %d\n", fd -> f_pos, count, cur_inode -> file_len);

    if (fd -> f_pos == cur_inode -> file_len)
        return 0;
    if (fd -> f_pos + count >= cur_inode -> file_len)
        count = cur_inode -> file_len - fd -> f_pos;
    //index of the first block we should read
    int first_block_index = fd -> f_pos / BLOCK_SIZE;
    int last_block_index = (fd -> f_pos + count) / BLOCK_SIZE;
    //we are reading a single block
    if (first_block_index == last_block_index)
    {
        //printk("Reading a single block.\n");
        get_data((uint64_t) cur_inode -> i_data[first_block_index], buf_ptr, fd -> f_pos % BLOCK_SIZE, count);
        fd -> f_pos += count;
        return count;
    }
    get_data((uint64_t) (cur_inode -> i_data[first_block_index]), buf_ptr, fd -> f_pos % BLOCK_SIZE, BLOCK_SIZE - fd -> f_pos % BLOCK_SIZE);
    buf_ptr += (BLOCK_SIZE - fd -> f_pos % BLOCK_SIZE);
    for (i = first_block_index + 1;i <= last_block_index - 1;++i)
    {
        get_data((uint64_t)cur_inode -> i_data[i], buf_ptr, 0, BLOCK_SIZE);
        buf_ptr += BLOCK_SIZE;
    }
    fd -> f_pos += count;
    get_data((uint64_t) cur_inode -> i_data[last_block_index], buf_ptr, 0, fd -> f_pos % BLOCK_SIZE);
    return count;
}

//write blocks into SATA; calls write_sata() which really writes data.
ssize_t write_sata_wrapper(struct file *fd, void *buf, int count)
{
    int i;
    uint64_t data_index;
    char *buf_ptr = (char *)buf;
    struct inode cur_inode_instance;
    struct inode* cur_inode = &cur_inode_instance;
    get_inode((uint64_t) fd -> inode, cur_inode);

    //printk("fd inode index: %d\n", (uint64_t)fd -> inode);
    //index of the first block we should read
    int first_block_index = fd -> f_pos / BLOCK_SIZE;
    int cur_last_block_index = cur_inode -> block_count - 1;
    int last_block_index = (fd -> f_pos + count) / BLOCK_SIZE;
    int new_block_count = last_block_index - cur_last_block_index;
    //printk("new block count: %d\n", new_block_count);
    //need to create new blocks for writing
    while (new_block_count > 0)
    {
        data_index = get_free_data();
        set_data_bitmap(data_index);
        //printk("new block allocated: %d\n", data_index);
        //no more free blocks
        if (data_index == -1)
        {
            printf("No more free blocks.\n");
            return -ENOSPC;
        }
        cur_inode -> i_data[cur_inode -> block_count++] = (int *)data_index;
        new_block_count--;
    }
    if (first_block_index == last_block_index)
    {
        //printk("writing the data block here.\n");
        write_data( (uint64_t)cur_inode -> i_data[first_block_index], buf_ptr, fd -> f_pos % BLOCK_SIZE, count);
        fd -> f_pos += count;
    }
    else
    {
        write_data((uint64_t) (cur_inode -> i_data[first_block_index]), buf_ptr, fd -> f_pos % BLOCK_SIZE, BLOCK_SIZE - fd -> f_pos % BLOCK_SIZE);
        buf_ptr += (BLOCK_SIZE - fd -> f_pos % BLOCK_SIZE);
        for (i = first_block_index + 1;i <= last_block_index - 1;++i)
        {
            write_data((uint64_t)cur_inode -> i_data[i], buf_ptr, 0, BLOCK_SIZE);
            buf_ptr += BLOCK_SIZE;
        }
        fd -> f_pos += count;
        write_data((uint64_t) cur_inode -> i_data[last_block_index], buf_ptr, 0, fd -> f_pos % BLOCK_SIZE);
    }
    if (fd -> f_pos >= cur_inode -> file_len)
        cur_inode -> file_len = fd -> f_pos;
    //printk("f_pos & count: %d %d %d\n", fd -> f_pos, count, cur_inode -> file_len);

    write_inode((uint64_t) fd -> inode, cur_inode);
    get_inode((uint64_t) fd -> inode, cur_inode);
    return count;
}

struct dentry_reader* open_root_dir(char *path)
{
    struct dentry_reader* cur_dentry_reader = malloc(sizeof(struct dentry_reader) );
    //a magic number for root
    cur_dentry_reader -> dentry = (struct dentry*)0x123456;
    cur_dentry_reader -> offset = 0;
    return cur_dentry_reader;
}
struct dentry* read_root_dir(struct dentry_reader* reader)
{
    //printf("!!!!%d\n", reader -> offset);
    struct dentry* cur_dentry = malloc(sizeof(struct dentry));
    if (reader -> offset == 2)
        return NULL;
    if (reader -> offset == 0)
        strcpy(cur_dentry -> d_iname, "/tarfs");
    else if (reader -> offset == 1)
        strcpy(cur_dentry -> d_iname, "/sata");
    reader -> offset++;
    return cur_dentry;
}

struct dentry_reader* open_sata_dir(char *path)
{
    uint64_t i;
    read_dentry_bitmap(dentry_bitmap);
    struct dentry_reader *dentry_reader_ptr = (struct dentry_reader*) malloc(sizeof(struct dentry_reader));
    struct dentry* cur_dentry = (struct dentry*) malloc(sizeof(struct dentry) );

    dentry_reader_ptr -> fs_type = 1;
    for (i = 0;i < BLOCK_SIZE;++i)
    {
        if (dentry_bitmap[i] == 0)
            continue;
        get_dentry(i, cur_dentry);
        if (!strcmp(path, cur_dentry -> d_iname) )
        {
            dentry_reader_ptr -> dentry = (struct dentry*) i;
            dentry_reader_ptr -> offset = 0;
            return dentry_reader_ptr;
        }
    }
    //printf("Cannot find current path.\n");
    return NULL;
}
struct dentry* read_sata_dir(struct dentry_reader *reader)
{
    struct dentry* cur_dentry = (struct dentry*) malloc(sizeof(struct dentry) );
    get_dentry((uint64_t)reader -> dentry, cur_dentry);
    //printf("next: %x\n", cur_dentry -> d_subdirs[reader -> offset]);
    if (cur_dentry -> d_subdirs[reader -> offset] == 0)
        return NULL;
    //printf("read sata dir.\n");
    get_dentry((uint64_t)cur_dentry -> d_subdirs[reader -> offset], cur_dentry);
    ++reader -> offset;
    return cur_dentry;
}

int close_sata_dir(struct dentry_reader* reader)
{
    free(reader);
    reader -> offset = 0;
    return 0;
}
void init_fs(void)
{
    common_buf = (char *)sendpage(0, 0, 0, 4096);
    memset(common_buf, 0, 4096);
    int i;
    struct response res;
    struct mem_req req;
    uint32_t bar5 = checkAllBuses();
    //map_page(bar5, KERN_VMBASE + bar5, 1, PTE_W);
    req.phys_addr = bar5;
    req.len = 8192;
    req.dest_addr = 0;
    int rd = port_connect(1, sizeof(req), &req);
    get_response(rd, &res);

    //abar = (HBA_MEM*) (KERN_VMBASE + bar5); 
    abar = res.buf;
    pdtable = probe_port(abar);
    for (i = 0;i < TOTAL_BLOCK_COUNT;++i)
        write_sata(pdtable[0], i, 0, 1, common_buf);

    struct super_block *super_block = &cur_super_block;
    super_block -> free_block_count = DATA_BLOCK_COUNT;
    super_block -> free_inode_count = INODE_COUNT;
    super_block -> inode_begin = INODE_START_INDEX;

    int inode_id = 0, dentry_id = 0, parent_dentry = -1;
    const char pathname[10] = "/";
    build_inode(inode_id, 0);
    build_dentry(inode_id, dentry_id, parent_dentry, pathname);
}

void ls_sata_fs(char *path)
{
    //printk("At ls.\n");
    struct dentry_reader* reader = open_sata_dir(path);
    struct dentry* tmp_dentry_reader;
    while ((tmp_dentry_reader = read_sata_dir(reader) ) != NULL)
    {
        printf("%s\n", tmp_dentry_reader -> d_iname);
    }
    close_sata_dir(reader);
}


