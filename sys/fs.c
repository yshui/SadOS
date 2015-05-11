#include <sys/printk.h>
#include <vfs.h>
#include <string.h>
#define BLOCK_SIZE  4096
#define FS_SIZE     (BLOCK_SIZE * 64)
#define INODE_START_ADDR    (4 * BLOCK_SIZE)
#define DENTRY_START_ADDR   (5 * BLOCK_SIZE)
#define DATA_START_ADDR     (6 * BLOCK_SIZE)
#define INODE_COUNT 80
#define BLOCK_COUNT 52

struct file temp_fd;
char fs[FS_SIZE];

//given index, get block from hard disk
char *get_block(int index)
{
    //for hard disk, return a block of data
    return &fs[index * BLOCK_SIZE];
}

void init_fs(void)
{
    int i;
    for (i = 0;i < FS_SIZE;++i)
        fs[i] = 0;
    struct super_block *super_block = (struct super_block *)fs;
    super_block -> free_block_count = 52;
    super_block -> free_inode_count = 80;
    super_block -> inode_begin = 3;
}

void strncpy(char *dest, char *source, int count)
{
    int i;
    for (i = 0;i < count;++i)
        dest[i] = source[i];
    dest[count] = '\0';
}

void read_inode_bitmap(char *inode_bitmap)
{
    strncpy(inode_bitmap, fs + BLOCK_SIZE, BLOCK_SIZE);
}
void read_data_bitmap(char *data_bitmap)
{
    strncpy(data_bitmap, fs + 2 * BLOCK_SIZE, BLOCK_SIZE);
}
void read_dentry_bitmap(char *dentry_bitmap)
{
    strncpy(dentry_bitmap, fs + 3 * BLOCK_SIZE, BLOCK_SIZE);
}

uint64_t get_free_inode(void)
{
    char inode_bitmap[BLOCK_SIZE];
    uint64_t i;

    read_inode_bitmap(inode_bitmap);
    for (i = 0;i < BLOCK_SIZE;++i)
        if (inode_bitmap[i] == 0)
            return i;
    return -1;
}
uint64_t get_free_dentry(void)
{
    char dentry_bitmap[BLOCK_SIZE];
    uint64_t i;

    read_dentry_bitmap(dentry_bitmap);
    for (i = 0;i < BLOCK_SIZE;++i)
        if (dentry_bitmap[i] == 0)
            return i;
    return -1;
}

uint64_t get_free_data(void)
{
    char data_bitmap[BLOCK_SIZE];
    uint64_t i;

    read_data_bitmap(data_bitmap);
    for (i = 0;i < BLOCK_SIZE;++i)
        if (data_bitmap[i] == 0)
            return i;
    return -1;
}

void set_inode_bitmap(int inode_id)
{
    *(fs + BLOCK_SIZE + inode_id) = 1;
}
void set_data_bitmap(int data_id)
{
    *(fs + 2 * BLOCK_SIZE + data_id) = 1;
}
void set_dentry_bitmap(int dentry_id)
{
    *(fs + 3 * BLOCK_SIZE + dentry_id) = 1;
}

void build_inode(int inode_id, uint16_t flags)
{
    struct inode *new_inode = (struct inode*) (fs + INODE_START_ADDR + inode_id * sizeof(struct inode));
    new_inode -> inode_id = inode_id;
    new_inode -> file_len = 0;
    new_inode -> block_count = 0;
    int i;
    //0: NULL block
    //deprecated
    for (i = 0;i < 15;++i)
        new_inode -> i_data[i] = 0;
    new_inode -> flags = flags;
    //set bitmap
    set_inode_bitmap(inode_id);
}

void build_dentry(uint64_t inode_id, uint64_t dentry_id, uint64_t d_parent, const char* d_iname)
{
    struct dentry *new_dentry = (struct dentry*) (fs + DENTRY_START_ADDR + dentry_id * sizeof(struct dentry) );
    new_dentry -> d_parent = (struct dentry*) d_parent;
    int i;
    for (i = 0;i < 30;++i)
        new_dentry -> d_subdirs[i] = 0;
    strcpy(new_dentry -> d_iname, d_iname);
    //actually stores inode ID
    new_dentry -> d_inode = (struct inode*) inode_id;
    //set bitmap
    set_dentry_bitmap(dentry_id);
}

void get_dentry_block(int inode_index, struct dentry* dentry_block)
{
    char *dentry_block_ptr = (char *)dentry_block;
    strncpy(dentry_block_ptr, &fs[DENTRY_START_ADDR + inode_index * sizeof(struct inode)], sizeof(struct dentry));
}

void get_inode_block(int inode_index, struct inode* inode_block)
{
    char *inode_block_ptr = (char *)inode_block;
    strncpy(inode_block_ptr, &fs[INODE_START_ADDR + inode_index * sizeof(struct inode)], sizeof(struct inode));
}

void get_data_block(uint64_t data_index, char *data_block)
{
    strncpy(data_block, &fs[DATA_START_ADDR + data_index * BLOCK_SIZE], BLOCK_SIZE);
}
void write_block(int data_index, char *data, int offset, int count)
{
    strncpy(&fs[DATA_START_ADDR + data_index * BLOCK_SIZE + offset], data, count);
}

int sata_get_parent(char *cur_dentry_name)
{
    int i = 0, j, len = strlen(cur_dentry_name);
    char parent_dir[200] = {0}, inode_bitmap[BLOCK_SIZE];
    struct dentry cur_dentry;
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

    read_inode_bitmap(inode_bitmap);
    for (i = 0;i < BLOCK_SIZE;++i)
    {
        if (inode_bitmap[i] == 0)
            continue;
        get_dentry_block(i, &cur_dentry);
        if (!strcmp(cur_dentry.d_iname, parent_dir) )
        {
            return cur_dentry.dentry_id;
        }
    }
    return -1;
}

struct file* sata_open(const char* pathname, int flags)
{
    int i;
    char inode_bitmap[BLOCK_SIZE];
    struct inode cur_inode;
    struct dentry cur_dentry;
    struct file *fd = &temp_fd;

    read_inode_bitmap(inode_bitmap);
    for (i = 0;i < BLOCK_SIZE;++i)
    {
        if (inode_bitmap[i] == 0)
            continue;
        get_inode_block(i, &cur_inode);
        get_dentry_block(i, &cur_dentry);

        if (!strcmp(cur_dentry.d_iname, pathname) )
        {
            fd -> inode = &cur_inode;
            strcmp(fd -> fname, pathname);
            fd -> f_flags = flags;
            fd -> f_pos = 0;
            return fd;
        }
    }
    //file does not exist
    if (flags != O_CREAT)
        printk("Error: file doesn't exist.\n");
    else
    {
        int parent_dentry = sata_get_parent(fd -> fname);
        if (parent_dentry == -1)
        {
            printk("Cannot find the file path.\n");
            return NULL;
        }
        uint64_t inode_id = get_free_inode();
        uint64_t dentry_id = get_free_dentry();
        build_inode(inode_id, flags);
        build_dentry(inode_id, dentry_id, parent_dentry, pathname);
    }
    return NULL;
}
//read blocks from SATA
int sata_read(struct file* fd, void *buf, int count)
{
    int i;
    int *data_block_loc;
    char *buf_ptr = (char *)buf;

    if (fd -> f_pos == fd -> inode -> file_len)
        return 0;
    if (fd -> f_pos + count >= fd -> inode -> file_len)
        count = fd -> inode -> file_len - fd -> f_pos;
    for (i = 0;i < fd -> inode -> block_count;++i)
        if (i * BLOCK_SIZE <= fd -> f_pos && (i + 1 ) * BLOCK_SIZE > fd -> f_pos)
        {
            data_block_loc = fd -> inode -> i_data[i];
            get_data_block((uint64_t)data_block_loc, buf_ptr);
            fd -> f_pos += count;
            break;
        }
    return count;
}

//write blocks into SATA
void sata_write(struct file *fd, char *buf, int count)
{
    int i;
    //check to see if there is enough space
    int last_block_size = fd -> inode -> file_len % BLOCK_SIZE;
    
    //not full
    if (last_block_size != BLOCK_SIZE)
    {
        if (BLOCK_SIZE - last_block_size < count)
        {
            write_block(fd -> inode -> inode_id, buf, last_block_size, count);
            buf += BLOCK_SIZE - last_block_size;
            count -= (BLOCK_SIZE - last_block_size);
        }
        else
        {
            write_block(fd -> inode -> inode_id, buf, last_block_size, BLOCK_SIZE - last_block_size);
            return;
        }
    }
    //write several blocks
    while (count > 0)
    {
        i = get_free_data();
        //no more free blocks
        if (i == -1)
        {
            printk("No more free blocks.\n");
            return;
        }
        //use a new block
        if (count >= BLOCK_SIZE)
        {
            write_block(fd -> inode -> inode_id, buf, 0, BLOCK_SIZE);
            count -= BLOCK_SIZE; 
            fd -> f_pos += BLOCK_SIZE;
        }
        else
        {
            write_block(fd -> inode -> inode_id, buf, 0, count);
            fd -> f_pos += count;
        }
    }
    fd -> inode -> file_len += count;
}

