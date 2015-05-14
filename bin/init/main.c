#include <syscall.h>
#include <ipc.h>
#include <uapi/mem.h>
#include <thread.h>
#include <elf/elf.h>
#include <string.h>
#include <as.h>
#include <bitops.h>
//.bss in init can't exceed 1MB
char X;
void bootstrap(int mem_rd){
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
const char q[30] = "Hello world from user space";
void fork_entry_here(void) {
	exit(1);
}
int main() {
	struct response res;
	struct mem_req req;
	req.type = MAP_PHYSICAL;
	req.phys_addr = 0xb8000;
	req.len = 80*25*2;
	int rd = port_connect(1, sizeof(req), &req);
	get_response(rd, &res);
	char *vbase = res.buf;
	int i;
	for (i=0; q[i]; i++) {
		vbase[i*2+4] = q[i];
		vbase[i*2+5] = 0x7;
	}
	syscall_1(0, 42);


	req.type = MAP_NORMAL;
	req.len = 0x1000;
	int rd2 = request(rd, sizeof(req), &req);
	get_response(rd2, &res);
	volatile uint8_t * volatile Addr = res.buf;
	Addr[0] = 'a';
	vbase[0] = Addr[0];
	vbase[1] = 7;

	bootstrap(rd);
	X='b';
	vbase[2]=X;
	vbase[3]=7;
	struct thread_info ti;
	get_thread_info(&ti);

	int as = asnew(AS_SNAPSHOT);
	ti.rcx = (uint64_t)&fork_entry_here;
	create_task(as, &ti);

	int tarfsd = port_connect(2, 0, NULL);
	get_response(tarfsd, &res);
	char *x = (void *)ALIGN((uint64_t)res.buf, 12);
	char *end = (void *)ALIGN_UP((uint64_t)res.buf+res.len, 12);
	int count = 0;
	while(x<end){
		count += *x;
		x += 4096;
	}

	exit(count);
	for(;;);
}
