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
void bootstrap(void){
	//The kernel doesn't really load elf properly
	//So let's bootstrap ourself
	struct elf_info *ei = elf_load((void *)0x400000);
	//Map .got
	struct elf_section_info *esi = elf_find_section(ei, ".got");
	memcpy((void *)esi->hdr->sh_addr, esi->section_base, esi->hdr->sh_size);
	free(esi);

	//Map .got.plt
	esi = elf_find_section(ei, ".got.plt");
	memcpy((void *)esi->hdr->sh_addr, esi->section_base, esi->hdr->sh_size);
	free(esi);

	//Map .data
	esi = elf_find_section(ei, ".data");
	memcpy((void *)esi->hdr->sh_addr, esi->section_base, esi->hdr->sh_size);
	free(esi);
	free(ei);
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
int main() {
	syscall_1(0, 42);
	bootstrap();
	X='b';
	struct thread_info ti;
	get_thread_info(&ti);

	struct response res;
	int tarfsd = port_connect(2, 0, NULL);
	get_response(tarfsd, &res);
	tar_begin = res.buf;
	tar_len = res.len;

	int as = asnew(AS_SNAPSHOT);
	ti.rcx = (uint64_t)&fork_entry_here;
	create_task(as, &ti, 0);

	wait_on_port(5);
	int pd = port_connect(5, 0, NULL);
	struct fd_set fds;
	fd_set_set(&fds, pd);
	wait_on(NULL, &fds, 0);
	dup2(pd, 0);
	dup2(pd, 1);

	int i = 0;
	while(1)
		printf("test%d\n", i);
}
