#include <ipc.h>
#include <uapi/mem.h>
const char q[30] = "Hello world from user space";
int main() {
	struct response res;
	struct mem_req req;
	req.phys_addr = 0xb8000;
	req.len = 80*25*2;
	req.dest_addr = 0;
	int rd = port_connect(1, sizeof(req), &req);
	get_response(rd, &res);
	char *vbase = res.buf;
	int i;
	for (i=0; q[i]; i++) {
		vbase[i*2+4] = q[i];
		vbase[i*2+5] = 0x7;
	}
	for (;;);
}
