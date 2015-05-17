#include <ipc.h>
#include <uapi/mem.h>
#include <uapi/portio.h>
#include <uapi/ioreq.h>
#include <stdlib.h>
#include <sendpage.h>
#include <bitops.h>
#include <errno.h>
#define R (25)
#define C (80)
#define VBUF_SIZE sizeof(buffer)
const char q[30] = "Hello world from user space";
static uint16_t video_port = 0;
static uint8_t pos_x, pos_y;
static uint8_t *voff = NULL;
static uint8_t *vbase = NULL;
static uint8_t buffer[4096];
static uint16_t offset;
static int portio_fd;
static void outb(uint16_t port, uint8_t byte) {
	struct portio_req preq;
	int cookie;
	preq.byte = byte;
	preq.type = 1;
	preq.port = port;
	cookie = request(portio_fd, sizeof(preq), &preq);
	close(cookie);
}
void set_cursor(void) {
	uint16_t pos = (pos_x*C)+pos_y;
	outb(video_port, 0x0f);
	outb(video_port+1, pos&0xff);
	outb(video_port, 0x0e);
	outb(video_port+1, (pos>>8)&0xff);
}
void blit(void) {
	int start = offset-(pos_y+(pos_x-1)*C)*2;
	if (start < 0)
		start += VBUF_SIZE;
	uint8_t *v = vbase;
	while(start != offset) {
		*(v++) = buffer[start];
		start = (start+1)%VBUF_SIZE;
	}
	voff = v;
	while(v-vbase <= R*C*2) {
		*(v++) = 0;
		*(v++) = 0x07;
	}
}

void line_feed(void) {
	while(pos_y < C) {
		//Clear rest of the line
		buffer[offset] = 0;
		offset = (offset+1)%VBUF_SIZE;
		buffer[offset] = 0x07;
		offset = (offset+1)%VBUF_SIZE;
		*(voff++) = 0;
		*(voff++) = 0x07;
		pos_y++;
	}
	pos_x++;
	pos_y = 0;
	if (pos_x >= R) {
		pos_x = R-1;
		blit();
	}
	set_cursor();
}

int vga_puts(const char *str) {
	int c = 0;
	while(*str) {
		switch(*str) {
		case '\n':
			line_feed();
			break;
		default:
			buffer[offset] = *str;
			offset = (offset+1)%VBUF_SIZE;
			buffer[offset] = 0x07;
			offset = (offset+1)%VBUF_SIZE;
			*(voff++) = *str;
			*(voff++) = 0x07;
			pos_y++;
			if (pos_y >= C)
				line_feed();
			break;
		}
		str++;
		c++;
	}
	set_cursor();
	return c;
}
int main() {
	struct response res;
	struct mem_req req;
	req.phys_addr = 0xb8000;
	req.len = 80*25*2;
	req.dest_addr = 0;
	int rd = port_connect(1, sizeof(req), &req);
	get_response(rd, &res);
	voff = vbase = res.buf;

	int bda_fd = port_connect(3, 0, NULL);
	get_response(bda_fd, &res);

	uint8_t *bda = res.buf;
	video_port = *(uint16_t *)(bda+0x463);
	close(bda_fd);

	portio_fd = port_connect(4, 0, NULL);
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
		fd_zero(&fds);
		fd_set_set(&fds, handle);
		wait_on(&fds, NULL, 0);
		int rhandle = pop_request(handle, &ureq);
		struct io_req *x = ureq.buf;
		struct io_res rx;
		if (x->type == IO_READ) {
			rx.err = EBADF;
			rx.len = 0;
			respond(rhandle, sizeof(rx), &rx);
		} else {
			vga_puts((void *)(x+1));
			rx.err = 0;
			rx.len = strlen((void *)(x+1));
			respond(rhandle, sizeof(rx), &rx);
		}
		uint64_t base = ALIGN((uint64_t)x, 12);
		munmap((void *)base, ureq.len);
	}
}
