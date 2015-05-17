#include <uapi/ioreq.h>
#include <stdlib.h>
#include <ipc.h>
#include <sendpage.h>
#include <bitops.h>

ssize_t read(int fd, void *buf, size_t len) {
	//Wait for write
	struct fd_set fds;
	fd_zero(&fds);
	fd_set_set(&fds, fd);
	wait_on(NULL, &fds, 0);

	struct io_req ioreq;
	ioreq.type = IO_READ;
	ioreq.len = len;

	int cookie = request(fd, sizeof(ioreq), &ioreq);
	fd_zero(&fds);
	fd_set_set(&fds, cookie);
	wait_on(&fds, NULL, 0);

	struct response res;
	get_response(cookie, &res);

	struct io_res iores;
	memcpy(&iores, res.buf, sizeof(iores));
	//unmap res.buf
	uint64_t base = ALIGN((uint64_t)res.buf, 12);
	if (iores.err) {
		errno = iores.err;
		munmap((void *)base, res.len);
		return -1;
	}
	memcpy(buf, res.buf+sizeof(struct io_res), len);
	munmap((void *)base, res.len);
	return iores.len;
}

ssize_t write(int fd, const void *buf, size_t len) {
	struct fd_set fds;
	fd_zero(&fds);
	fd_set_set(&fds, fd);
	wait_on(NULL, &fds, 0);

	size_t allocated = ALIGN_UP(len+sizeof(struct io_req), 12);
	struct io_req *ioreq = sendpage(0, 0, 0, allocated);
	ioreq->type = IO_WRITE;
	ioreq->len = len;
	memcpy(ioreq+1, buf, len);

	int cookie = request(fd, sizeof(struct io_req)+len, ioreq);
	fd_zero(&fds);
	fd_set_set(&fds, cookie);
	wait_on(&fds, NULL, 0);

	struct response res;
	get_response(cookie, &res);

	struct io_res iores;
	memcpy(&iores, res.buf, sizeof(iores));
	//unmap res.buf
	uint64_t base = ALIGN((uint64_t)res.buf, 12);
	munmap((void *)base, res.len);
	if (iores.err) {
		errno = iores.err;
		return -1;
	}
	munmap(ioreq, allocated);
	return iores.len;
}
