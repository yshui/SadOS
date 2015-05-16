#include <ipc.h>
#include <uapi/mem.h>
#include <stdlib.h>
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

	long pd = open_port(5);
	struct fd_set fds;
	fds.nfds = 0;
	fd_set_set(&fds, pd);
	wait_on(&fds, NULL, 0);

	struct urequest ureq;
	int handle = pop_request(pd, &ureq);

	while(1) {
		fds.nfds = 0;
		fd_set_set(&fds, handle);
		wait_on(&fds, NULL, 0);
		int rhandle = pop_request(handle, &ureq);
		close(rhandle);
		char *x = ureq.buf;
		for (i=0; x[i]; i++) {
			vbase[160+i*2] = x[i];
			vbase[161+i*2] = 7;
		}
	}
}
