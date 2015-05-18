#include <uapi/ioreq.h>
#include <stdlib.h>
#include <ipc.h>
#include <sendpage.h>
#include <bitops.h>
#include <stdio.h>
#include <vfs.h>

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
extern char __cwd[];
int open(const char *path, int flags) {
	char *allocated = NULL;
	if (*path != '/') {
		int cwd_len = strlen(__cwd);
		allocated = malloc(cwd_len+strlen(path)+1);
		strcpy(allocated, __cwd);
		strcpy(allocated+cwd_len, path);
		path = allocated;
	}
	struct fd_set fds;
	int handle = port_connect(6, 0, NULL);
	if (handle < 0)
		return handle;
	fd_zero(&fds);
	fd_set_set(&fds, handle);
    printf("open path: %s\n", path);
	int ret = wait_on(NULL, &fds, 0);
    printf("ret: %s\n", ret);
	if (ret < 0)
		return ret;

	struct io_req *req = malloc(sizeof(struct io_req)+strlen(path)+1);
	req->flags = flags;
	req->type = IO_OPEN;
	req->len = strlen(path);
	strcpy((void *)(req+1), path);
	int cookie = request(handle, sizeof(struct io_req)+strlen(path)+1, req);
	if (allocated) {
		free(allocated);
		allocated = NULL;
	}
	if (cookie < 0)
		return cookie;

	fd_zero(&fds);
	fd_set_set(&fds, cookie);
	ret = wait_on(&fds, NULL, 0);
	if (ret < 0)
		return ret;

	struct response res;
	ret = get_response(cookie, &res);
	if (ret < 0)
		return ret;

	struct io_res *x = res.buf;
	int tmp = x->err;

	uint64_t base = ALIGN((uint64_t)x, 12);
	munmap((void *)base, res.len);

	if (tmp != 0) {
		errno = tmp;
		return -1;
	}
	return handle;
}
int opendir(char *path) {
	char *allocated = NULL;
	if (*path != '/') {
		int cwd_len = strlen(__cwd);
		allocated = malloc(cwd_len+strlen(path)+1);
		strcpy(allocated, __cwd);
		strcpy(allocated+cwd_len, path);
		path = allocated;
	}
	struct fd_set fds;
	int handle = port_connect(6, 0, NULL);
	if (handle < 0)
		return handle;
	fd_zero(&fds);
	fd_set_set(&fds, handle);
    //printf("open dir: %s\n", path);
	int ret = wait_on(NULL, &fds, 0);
	if (ret < 0)
		return ret;

	struct io_req *req = malloc(sizeof(struct io_req)+strlen(path)+1);
	req->flags = 0;
	req->type = IO_OPENDIR;
	req->len = strlen(path);
	strcpy((void *)(req+1), path);
	int cookie = request(handle, sizeof(struct io_req)+strlen(path)+1, req);
	if (allocated) {
		free(allocated);
		allocated = NULL;
	}
	if (cookie < 0)
		return cookie;

	fd_zero(&fds);
	fd_set_set(&fds, cookie);
	ret = wait_on(&fds, NULL, 0);
	if (ret < 0)
		return ret;

	struct response res;
	ret = get_response(cookie, &res);
	if (ret < 0)
		return ret;

	struct io_res *x = res.buf;
	int tmp = x->err;

	uint64_t base = ALIGN((uint64_t)x, 12);
	munmap((void *)base, res.len);

	if (tmp != 0) {
		errno = tmp;
		return -1;
	}
	return handle;
}

int readdir(int fd, void *buf) {
	struct fd_set fds;
	fd_zero(&fds);
	fd_set_set(&fds, fd);
	wait_on(NULL, &fds, 0);

	struct io_req ioreq;
	ioreq.type = IO_READDIR;
	ioreq.len = 1;

	int cookie = request(fd, sizeof(ioreq), &ioreq);
	fd_zero(&fds);
	fd_set_set(&fds, cookie);
	wait_on(&fds, NULL, 0);

	struct response res;
	get_response(cookie, &res);

	struct io_res *iores = (struct io_res*)malloc(4096);
	memcpy(iores, res.buf, sizeof(iores) + sizeof(struct dentry));
	//unmap res.buf
	uint64_t base = ALIGN((uint64_t)res.buf, 12);
	if (iores->err) {
		errno = iores->err;
		munmap((void *)base, res.len);
		return -1;
	}
	memcpy(buf, res.buf+sizeof(struct io_res), sizeof(struct dentry));
	munmap((void *)base, res.len);
	return iores -> len;
}
