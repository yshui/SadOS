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
//.bss in init can't exceed 1MB
char X;
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
int main() {
	bootstrap();
	X='b';
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
	printf("%d\n", pid);
	int pid2 = fork();
	if (pid2 == 0)
		fork_fs();
	wait_on_port(6);
	int fd = open("/tarfs/bin/init", O_RDONLY);
	printf("Open %d\n", fd);
	char *buf = malloc(1024);
	read(fd, buf, 4);
	buf[4] = 0;
	printf("%s\n", buf+1);
	while(1) {
		read(0, buf, 1024);
		printf("%s\n", buf);
	}
	return 0;
}
