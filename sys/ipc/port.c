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

static struct port_ops *port[MAX_PORT];

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
	disable_interrupts();
	//Validate user addr
	printk("port_connect(%d, %d, %p)\n", port_number, len, buf);
	if (buf || len) {
		int ret = validate_range((uint64_t)buf, len);
		if (ret) {
			enable_interrupts();
			return -EINVAL;
		}
	}

	struct request *req = obj_pool_alloc(current->file_pool);
	req->type = REQ_REQUEST;
	req->waited = false;
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
	reqc->waited = false;
	reqc->owner = req;
	int fd = fdtable_insert(current->fds, reqc);
	if (fd < 0) {
		obj_pool_free(current->file_pool, reqc);
		enable_interrupts();
		return -EMFILE;
	}

	enable_interrupts();

	int ret = req->rops->request(reqc, len, buf);
	if (ret != 0) {
		obj_pool_free(current->file_pool, req);
		current->fds->file[fd] = NULL;
		return ret;
	}
	printk("Return\n");
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

void wait_on_requesst(struct request *req, struct task *t) {
	t->state = TASK_WAITING;
	req->waited = true;
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
