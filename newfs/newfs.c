#include "../include/sys/defs.h"
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

struct superblock {
	uint32_t magic;
};
struct inode {
	uint64_t refcount;
};

int main(int argc, char* argv[]) {
	struct superblock* sb;
	int fd;
	struct sector { char contents[512]; }* disk;
	struct inode* ino;
	struct stat diskstat;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s disk.img\n", argv[0]);
		return 0;
	}

	if (stat(argv[1], &diskstat) == -1) {
		fprintf(stderr, "%s: Unable to stat %s (%s)\n", argv[0], argv[1], strerror(errno));
		return 0;
	}

	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "%s: Unable to open %s (%s)\n", argv[0], argv[1], strerror(errno));
		return 0;
	}

	disk = mmap(NULL, diskstat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (disk == MAP_FAILED) {
		fprintf(stderr, "%s: Unable to map %s (%s)\n", argv[0], argv[1], strerror(errno));
		return 0;
	}

	sb = (struct superblock*)&disk[0];
	sb->magic = 0x20363035;
	ino = (struct inode*)&disk[1];
	ino->refcount = 0x45444F4344414544;

	munmap(disk, diskstat.st_size);
	close(fd);
	return 0;
}
