#include <syscall.h>
#include <ipc.h>
#include <uapi/mem.h>
#include <thread.h>
#include <elf/elf.h>
#include <string.h>
#include <as.h>
#include <bitops.h>
#include <sendpage.h>
#include <sys/tar.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs.h>
//.bss in init can't exceed 1MB
char X;
char *_environ = NULL;
void clear_free(void);
extern char __cwd[];
void bootstrap(void){
	//The kernel doesn't really load elf properly
	//So let's bootstrap ourself
	//Clear .bss range
	clear_free();
	struct elf_info *ei = elf_load((void *)0x400000);
	struct elf_section_info *esi = elf_find_section(ei, ".bss");
	memset((void *)esi->hdr->sh_addr, 0, esi->hdr->sh_size);
	free(esi);
	free(ei);

	strcpy(__cwd, "/");
	environ = &_environ;
}
static void *tar_begin;
static size_t tar_len;
static inline uint64_t atoi_oct(const char *str) {
	uint64_t ans = 0;
	while (*str) {
		ans = ans*8+*str-'0';
		str++;
	}
	return ans;
}
void fork_entry_here(void) {
	struct tar_header *thdr = tar_begin;
	while ((void *)thdr < tar_begin+tar_len) {
		char *n = thdr->name;
		if (*n == '/')
			n++;
		if (strcmp(n, "bin/console") == 0)
			break;
		uint64_t size = atoi_oct(thdr->size);
		thdr = (void *)((char *)thdr+512+ALIGN_UP(size, 9));
	}
	char *argv = NULL;
	char *env = NULL;
	execvm((char *)thdr+512, &argv, &env);
	for(;;); //Shouldn't be reached
}
void fork_fs(void) {
	struct tar_header *thdr = tar_begin;
	while ((void *)thdr < tar_begin+tar_len) {
		char *n = thdr->name;
		if (*n == '/')
			n++;
		if (strcmp(n, "bin/fs") == 0)
			break;
		uint64_t size = atoi_oct(thdr->size);
		thdr = (void *)((char *)thdr+512+ALIGN_UP(size, 9));
	}
	char *argv = NULL;
	char *env = NULL;
	execvm((char *)thdr+512, &argv, &env);
	for(;;); //Shouldn't be reached
}
void ls(const char *pathname)
{
    //printf("In ls here: %s\n", pathname);
    int ret;
    int fd = opendir((char*)pathname);
    //struct dentry_reader *p = NULL;
    struct dentry *de = (struct dentry*) malloc(sizeof(struct dentry));
    if (fd < 0)
    {
        printf("Cannot open directory %s.\n", pathname);
        return;
    }
    do {
        ret = readdir(fd, (void*)de);
        //printf("ret : %d\n", ret);
        if (ret)
            printf("%s\n", de->d_iname);
    }
    while(ret);
}

void cat(char* pathname)
{
    //printf("Real path name for cat: %s\n", cur_dir);
    int fd = open(pathname, 0);
    if (fd < 0)
    {
        printf("Error: file doesn't exist\n");
        return;
    }
    uint64_t fd_int = (uint64_t) fd;
    printf("fd: %d\n", fd);
    char buf[513];
    int count;

    while ( (count = read(fd_int, buf, 513) ) != 0)
        printf("%s", buf);
    printf("\n");
}

int main() {
	bootstrap();
	X='b';
	putenv("PATH=/tarfs/bin");
	struct thread_info ti;
	get_thread_info(&ti);

	struct response res;
	int tarfsd = port_connect(2, 0, NULL);
	get_response(tarfsd, &res);
	tar_begin = res.buf;
	tar_len = res.len;

	int pid = fork();
	if (pid == 0)
		fork_entry_here();


	wait_on_port(5);
	int pd = port_connect(5, 0, NULL);
	struct fd_set fds;
	fd_set_set(&fds, pd);
	wait_on(NULL, &fds, 0);
	dup2(pd, 0);
	dup2(0, 1);
	//printf("%d\n", pid);
	int pid2 = fork();
	if (pid2 == 0)
		fork_fs();
	wait_on_port(6);
//    int fd = open("/tarfs/test/f", 0);
//    int fd1 = open("/sata/f", O_CREAT);
//    printf("fd assigned: %d\n", fd);
//    char buf[513];
//    while (read(fd, buf, 512) != 0)
//    {
//        //printf("%s\n", buf);
//        write(fd1, buf, 512);
//    }
//    ls("/sata");
//    cat("/sata/f");
//
	if (fork() == 0)
		execve("/tarfs/bin/sbush", NULL, environ);
	wait_on(NULL, NULL, 0);
	return 0;
}
