/* No Copyright */
/* Public Domain */
#ifndef _TARFS_H
#define _TARFS_H
#include <vfs.h>

extern char _binary_tarfs_start;
extern char _binary_tarfs_end;

struct posix_header_ustar {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag[1];
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char pad[12];
};

void tarfs_init();
void ls_tarfs();
struct file* tarfs_open(const char* pathname, int flags);
int tarfs_read(struct file *fd, void *buf, int count);
struct dentry* read_tarfs_dir(struct dentry_reader* reader);
struct dentry_reader* open_tarfs_dir(char* path);
int close_tarfs_dir(struct dentry_reader* reader);

#endif
