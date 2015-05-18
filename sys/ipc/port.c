/* Copyright (C) 2015, Yuxuan Shui <yshuiv7@gmail.com> */

/*
 * Everyone is allowed to copy and/or redistribute verbatim
 * copies of this code as long as the following conditions
 * are met:
 *
 * 1. The copyright infomation and license is unchanged.
 *
 * 2. This piece of code is not submitted as or as part of
 *    an assignment to the CSE506 course at Stony Brook
 *    University, unless the submitter is the copyright
 *    holder.
 */
#include <sys/defs.h>
#include <sys/printk.h>
#include <sys/list.h>
#include <sys/port.h>
#include <bitops.h>
#include <errno.h>
#include <sys/mm.h>
#include <sys/syscall.h>
#include <sys/sched.h>
#include <sys/interrupt.h>
#include <sys/copy.h>

struct port_ops *port[MAX_PORT];

struct uresponse {
	void *buf;
	size_t len;
	struct list_node next;
};

struct task_list {
	struct task *task;
	struct list_node next;
};


SYSCALL(3, port_connect, int, port_number, size_t, len, void *, buf) {
	//Validate user addr
	printk("port_connect(%d, %d, %p)\n", port_number, len, buf);
	if (!port[port_number])
		return -ENOENT;
	disable_interrupts();
	if (buf || len) {
		int ret = validate_range((uint64_t)buf, len);
		if (ret) {
			enable_interrupts();
			return -EINVAL;
		}
	}

	struct request *req = obj_pool_alloc(current->file_pool);
	req->type = REQ_REQUEST;
	req->waited_rw = 0;
	req->owner = current;
	int fd = fdtable_insert(current->fds, req);
	if (fd < 0) {
		obj_pool_free(current->file_pool, req);
		enable_interrupts();
		return -EMFILE;
	}

	enable_interrupts();

	int ret = port[port_number]->connect(port_number, req, len, buf);
	if (ret != 0) {
		obj_pool_free(current->file_pool, req);
		current->fds->file[fd] = NULL;
		return ret;
	}
	printk("Return\n");
	return fd;
}

SYSCALL(3, request, int, rd, size_t, len, void *, buf) {
	disable_interrupts();
	printk("request(%d, %d, %p)\n", rd, len, buf);
	if (rd < 0 || rd > current->fds->max_fds || !current->fds->file[rd]) {
		enable_interrupts();
		return -EINVAL;
	}
	struct request *req = current->fds->file[rd];
	if (req->type != REQ_REQUEST) {
		enable_interrupts();
		return -EBADF;
	}
	if (!req->rops->request) {
		enable_interrupts();
		return -EBADF;
	}

	//Validate user addr
	if (len || buf) {
		int ret = validate_range((uint64_t)buf, len);
		if (ret) {
			enable_interrupts();
			return -EFAULT;
		}
	}

	struct request *reqc = obj_pool_alloc(current->file_pool);
	reqc->type = REQ_COOKIE;
	reqc->waited_rw = 0;
	reqc->owner = current;
	int fd = fdtable_insert(current->fds, reqc);
	if (fd < 0) {
		obj_pool_free(current->file_pool, reqc);
		enable_interrupts();
		return -EMFILE;
	}


	int ret = req->rops->request(req, reqc, len, buf);
	if (ret != 0) {
		obj_pool_free(current->file_pool, req);
		current->fds->file[fd] = NULL;
		enable_interrupts();
		return ret;
	}
	printk("Return\n");
	enable_interrupts();
	return fd;
}

SYSCALL(2, get_response, int, cookie, struct response *, res) {
	disable_interrupts();
	printk("get_response(cookie=%d)\n", cookie);
	if (validate_range((uint64_t)res, sizeof(struct response))) {
		enable_interrupts();
		return -EFAULT;
	}
	if (cookie < 0 || cookie > current->fds->max_fds || !current->fds->file[cookie]) {
		enable_interrupts();
		return -EBADF;
	}
	struct request *req = current->fds->file[cookie];
	if (!req->rops->get_response) {
		enable_interrupts();
		return -EBADF;
	}

	struct response xres;

	long ret = req->rops->get_response(req, &xres);
	if (ret != 0)
		goto end;
	if (req->type == REQ_COOKIE) {
		current->fds->file[cookie] = NULL;
		obj_pool_free(current->file_pool, req);
	}
	ret = copy_to_user_simple(&xres, res, sizeof(struct response));
end:
	enable_interrupts();
	printk("return\n");
	return ret;

}

SYSCALL(1, close, int, fd) {
	if (fd < 0 || fd > current->fds->max_fds || !current->fds->file[fd])
		return -EBADF;
	struct request *req = current->fds->file[fd];
	current->fds->file[fd] = NULL;
	if (req->rops->drop)
		req->rops->drop(req);
	obj_pool_free(current->file_pool, req);
	return 0;
}

int register_port(int port_number, struct port_ops *pops) {
	if (port[port_number])
		return -EBUSY;
	if (!pops->connect)
		panic("Registering port without connect()");
	port[port_number] = pops;
	return 0;
}

void deregister_port(int port_number) {
	port[port_number] = NULL;
}
static int fds_check(int rw, struct fd_set *fds) {
	int i;
	for (i = 0; i < fds->nfds; i++) {
		if (!fd_is_set(fds, i))
			continue;
		struct request *req = current->fds->file[i];
		if (!req) {
			printk("Bad file descriptor, %d\n", i);
			return -EBADF;
		}
		if (!req->rops->available) {
			printk("File descriptor doesn't allow wait_on\n");
			return -EBADF;
		}
		if (!req->rops->available(req, rw)) {
			printk("File %d is not available for %s\n", i, rw == 1 ? "read" : "write");
			fd_clear(fds, i);
		} else
			printk("File %d is available for %s\n", i, rw == 1 ? "read" : "write");
		req->waited_rw |= rw;
	}
	return 0;
}

SYSCALL(3, wait_on, void *, rbuf, void *, wbuf, int, timeout) {
	struct fd_set rfds, wfds;
	disable_interrupts();
	printk("Entering wait_on\n");
	int ret;
	fd_zero(&rfds);
	fd_zero(&wfds);
	if (rbuf) {
		ret = copy_from_user_simple(rbuf, &rfds, sizeof(rfds));
		if (ret != 0) {
			enable_interrupts();
			return -EFAULT;
		}
		if (rfds.nfds > current->fds->max_fds) {
			enable_interrupts();
			return -EINVAL;
		}
	}
	if (wbuf) {
		ret = copy_from_user_simple(wbuf, &wfds, sizeof(wfds));
		if (ret != 0) {
			enable_interrupts();
			return -EFAULT;
		}
		if (wfds.nfds > current->fds->max_fds) {
			enable_interrupts();
			return -EINVAL;
		}
	}

	current->state = TASK_WAITING;

	struct fd_set rfds2, wfds2;
	while (true) {
		//Change task state
		//Set request->waited
		memcpy(&rfds2, &rfds, sizeof(rfds));
		memcpy(&wfds2, &wfds, sizeof(wfds));
		ret = fds_check(1, &rfds2);
		if (ret != 0) {
			current->state = TASK_RUNNING;
			break;
		}
		ret = fds_check(2, &wfds2);
		if (ret != 0) {
			current->state = TASK_RUNNING;
			break;
		}
		if (!fd_set_empty(&rfds2) || !fd_set_empty(&wfds2)) {
			printk("Some fd is available, return\n");
			current->state = TASK_RUNNING;
			break;
		}
		current->state = TASK_WAITING;
		enable_interrupts();
		printk("No fd available, schedule()\n");
		schedule();
		printk("schedule() returned\n");
		disable_interrupts();
	}
	for (int i = 0; i < rfds.nfds; i++) {
		if (!fd_is_set(&rfds, i))
			continue;
		struct request *req = current->fds->file[i];
		if (!req)
			continue;
		req->waited_rw = 0;
	}
	for (int i = 0; i < wfds.nfds; i++) {
		if (!fd_is_set(&wfds, i))
			continue;
		struct request *req = current->fds->file[i];
		if (!req)
			continue;
		req->waited_rw = 0;
	}
	if (rbuf && ret == 0) {
		ret = copy_to_user_simple(&rfds2, rbuf, sizeof(struct fd_set));
		if (ret != 0)
			ret = -EFAULT;
	}
	if (wbuf && ret == 0) {
		ret = copy_to_user_simple(&wfds2, wbuf, sizeof(struct fd_set));
		if (ret != 0)
			ret = -EFAULT;
	}
	enable_interrupts();
	return ret;
}

SYSCALL(2, dup0, int, old_fd, int, new_fd) {
	if (old_fd < 0 || old_fd >= current->fds->max_fds || !current->fds->file[old_fd])
		return -EBADF;
	if (new_fd >= 0 && new_fd >= current->fds->max_fds)
		return -EBADF;
	if (new_fd >= 0 && current->fds->file[new_fd])
		return -EBUSY;

	struct request *req = current->fds->file[old_fd];
	int fd;
	struct request *new_req = obj_pool_alloc(current->file_pool);
	new_req->owner = current;
	new_req->waited_rw = 0;
	new_req->type = req->type;
	if (new_fd < 0) {
		fd = fdtable_insert(current->fds, new_req);
		if (fd < 0) {
			obj_pool_free(current->file_pool, new_req);
			return -EMFILE;
		}
	} else {
		fd = new_fd;
		current->fds->file[new_fd] = new_req;
	}

	int ret = 0;
	if (req->rops->dup)
		ret = req->rops->dup(req, new_req);
	if (ret != 0) {
		current->fds->file[fd] = NULL;
		obj_pool_free(current->file_pool, new_req);
		return ret;
	}
	return fd;
}
