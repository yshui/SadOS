#include <sys/port.h>
#include <errno.h>
#include <sys/interrupt.h>
#include <sys/syscall.h>
#include <sys/copy.h>
#include <sys/aspace.h>
struct uport {
	struct list_head incoming;
	int port_number;
};

struct uport_req {
	struct list_head clients;
	struct list_head incoming;
};

struct uport_cookie {
	struct request *client_req;
	struct page_list_head *response;
	struct task *owner;
};

struct message {
	struct page_list_head *plh;
	struct request *client_req;
	struct list_node next;
};

static struct list_head port_wait_list[MAX_PORT];
extern struct port_ops *port[];
struct port_ops uport_pops;
struct req_ops uport_server_rops;
struct req_ops uport_client_rops;

struct request *uport[MAX_PORT];

void port_init(void) {
	for (int i = 0; i < MAX_PORT; i++)
		list_head_init(&port_wait_list[i]);
}

SYSCALL(1, wait_on_port, int, port_number) {
	printk("wait on port %d", port_number);
	disable_interrupts();
	current->state = TASK_WAITING;
	while(1) {
		if (port[port_number]) {
			printk("port %d become available\n", port_number);
			break;
		}
		list_add(&port_wait_list[port_number], &current->tasks);
		enable_interrupts();
		schedule();
		disable_interrupts();
	}
	enable_interrupts();
	return 0;
}

SYSCALL(1, open_port, int, port_number) {
	disable_interrupts();
	int ret = register_port(port_number, &uport_pops);
	if (ret < 0) {
		enable_interrupts();
		return ret;
	}

	struct request *req = obj_pool_alloc(current->file_pool);
	ret = fdtable_insert(current->fds, req);
	if (ret < 0) {
		obj_pool_free(current->file_pool, req);
		enable_interrupts();
		return -EMFILE;
	}

	req->type = REQ_PORT;
	req->owner = current;
	req->data = obj_pool_alloc(current->file_pool);
	req->rops = &uport_server_rops;
	uport[port_number] = req;

	struct uport *up = req->data;
	list_head_init(&up->incoming);
	up->port_number = port_number;

	register_port(port_number, &uport_pops);

	struct task *ti, *tnxt;
	list_for_each_safe(&port_wait_list[port_number], ti, tnxt, tasks) {
		list_del(&ti->tasks);
		wake_up(ti);
	}
	enable_interrupts();
	return ret;
}

SYSCALL(2, pop_request, int, rd, void *, buf) {
	disable_interrupts();
	long ret = -EBADF;
	if (rd < 0 || rd > current->fds->max_fds || !current->fds->file[rd])
		goto end;
	struct request *req = current->fds->file[rd];
	if (req->type != REQ_PORT && req->type != REQ_PORT_REQ)
		goto end;

	ret = -EAGAIN;
	struct message *msg;
	struct urequest res;
	if (req->type == REQ_PORT) {
		struct uport *up = req->data;
		if (list_empty(&up->incoming))
			goto end;
		msg = list_top(&up->incoming, struct message, next);
	} else {
		struct uport_req *upreq = req->data;
		if (!list_empty(&upreq->incoming))
			msg = list_top(&upreq->incoming, struct message, next);
		else if (list_empty(&upreq->clients)) {
			//All clients are gone
			res.buf = NULL;
			res.len = 0;
			res.pid = -1;
			if (copy_to_user_simple(&res, buf, sizeof(res)) < 0) {
				ret = -EFAULT;
				goto end;
			}
			ret = 0;
			goto end;
		} else
			//No request available
			goto end;
	}

	struct request *sreq = obj_pool_alloc(current->file_pool);
	long fd = fdtable_insert(current->fds, sreq);
	if (fd < 0) {
		ret = -EMFILE;
		goto end;
	}

	uint64_t vaddr = 0;
	if (msg->plh) {
		vaddr = map_to_user(msg->plh, 0)+msg->plh->start_offset;
		if ((long)vaddr < 0) {
			ret = (long)vaddr;
			goto end;
		}
		res.buf = (void *)vaddr;
		res.len = msg->plh->npage*PAGE_SIZE;
	} else {
		res.buf = NULL;
		res.len = 0;
	}

	struct request *creq = msg->client_req;
	ret = copy_to_user_simple(&res, buf, sizeof(res));
	if (ret != 0) {
		ret = -EFAULT;
		remove_vma_range(current->as, vaddr, res.len);
		goto end;
	}
	list_del(&msg->next);
	if (msg->plh)
		page_list_free(msg->plh);
	struct task *client_task = msg->client_req->owner;
	obj_pool_free(client_task->file_pool, msg);

	if (req->type == REQ_PORT) {
		struct uport_req *upreq = obj_pool_alloc(current->file_pool);
		list_head_init(&upreq->incoming);
		list_head_init(&upreq->clients);
		list_add(&upreq->clients, &creq->next_req);
		sreq->data = upreq;
		sreq->type = REQ_PORT_REQ;
		creq->data = sreq;
	} else {
		//REQ_PORT_REQ
		struct uport_cookie *upcookie = obj_pool_alloc(current->file_pool);
		upcookie->client_req = creq;
		upcookie->response = NULL;
		upcookie->owner = current;
		sreq->data = upcookie;
		sreq->type = REQ_PORT_COOKIE;
		creq->data = upcookie;
	}

	sreq->rops = &uport_server_rops;
	sreq->owner = current;
	sreq->waited_rw = 0;
	creq->state = REQ_ESTABLISHED;
	if (creq->waited_rw&2)
		wake_up(creq->owner);

	ret = fd;
end:
	enable_interrupts();
	return ret;
}

SYSCALL(3, respond, int, fd, size_t, len, void *, buf) {
	disable_interrupts();
	long ret;
	ret = -EBADF;
	if (fd < 0 || fd >= current->fds->max_fds || !current->fds->file[fd])
		goto end;
	struct request *req = current->fds->file[fd];
	if (req->type != REQ_PORT_COOKIE)
		goto end;

	ret = -EFAULT;
	struct page_list_head *plh = map_from_user(buf, len);
	if (PTR_IS_ERR(plh))
		goto end;

	current->fds->file[fd] = NULL;
	obj_pool_free(current->file_pool, req);

	struct uport_cookie *upcookie = req->data;
	if (plh)
		upcookie->response = plh;
	else
		upcookie->response = (void *)1;
	if (upcookie->client_req->waited_rw&1)
		wake_up(upcookie->client_req->owner);
end:
	enable_interrupts();
	return ret;
}

int uport_connect(int port, struct request *req, size_t len, void *buf) {
	struct page_list_head *plh = NULL;
	if (buf || len) {
		plh = map_from_user(buf, len);
		if (PTR_IS_ERR(plh))
			return -EFAULT;
	}

	struct message *msg = obj_pool_alloc(current->file_pool);
	msg->plh = plh;
	msg->client_req = req;

	struct uport *up = uport[port]->data;
	req->data = NULL;
	req->rops = &uport_client_rops;
	req->state = REQ_PENDING;
	list_add_tail(&up->incoming, &msg->next);
	if (uport[port]->waited_rw&1)
		wake_up(uport[port]->owner);
	return 0;
}

int uport_request(struct request *req, size_t len, void *buf) {
	if (req->state == REQ_CLOSED)
		//Server side has closed connection
		return -EPIPE;
	if (req->state == REQ_PENDING)
		//Server side has not acknowledged this connection
		return -EAGAIN;

	struct request *sreq = req->data;
	struct page_list_head *plh = NULL;
	if (buf || len) {
		map_from_user(buf, len);
		if (PTR_IS_ERR(plh))
			return -EFAULT;
	}
	struct message *msg = obj_pool_alloc(current->file_pool);
	msg->client_req = req;
	msg->plh = plh;
	req->data = msg;
	req->state = REQ_PENDING;

	struct uport_req *upreq = sreq->data;
	list_add_tail(&upreq->incoming, &msg->next);

	if (sreq->waited_rw&1)
		//Wake up process
		wake_up(sreq->owner);
	return 0;
}

int uport_get_response(struct request *req, struct response *res) {
	if (req->type != REQ_COOKIE)
		return -EBADF;
	if (req->data == (void *)1) {
		//Server side has closed connection
		res->buf = NULL;
		res->len = 0;
		return 0;
	}
	if (req->state == REQ_PENDING)
		//Server side has not acknowledged this connection
		return -EAGAIN;

	struct request *sreq = req->data;
	struct uport_cookie *upcookie = sreq->data;

	if (!upcookie->response)
		return -EAGAIN;

	uint64_t vaddr = map_to_user(upcookie->response, 0);
	if ((long)vaddr < 0)
		return -ENOMEM;

	res->buf = (void *)(vaddr+upcookie->response->start_offset);
	res->len = upcookie->response->npage*PAGE_SIZE;
	return 0;
}

int uport_dup(struct request *old_req, struct request *new_req) {
	if (old_req->type != REQ_REQUEST)
		return -EBADF;
	if (old_req->state != REQ_ESTABLISHED)
		return -EAGAIN;
	new_req->data = old_req->data;
	new_req->state = old_req->state;

	struct request *sreq= old_req->data;
	struct uport_req *upreq = sreq->data;
	list_add(&upreq->clients, &new_req->next_req);
	return 0;
}

void uport_user_close(struct request *req) {
	if (req->state == REQ_PENDING) {
		struct message *msg = req->data;
		list_del(&msg->next);
		if (msg->plh)
			page_list_free(msg->plh);

		struct task *client_task = msg->client_req->owner;
		obj_pool_free(client_task->file_pool, msg);
		return;
	}

	struct uport_cookie *upcookie;
	switch(req->type) {
		case REQ_REQUEST:
			list_del(&req->next_req);
			break;
		case REQ_COOKIE:
			upcookie = req->data;
			if (upcookie->response) {
				//Server already replies, the sreq doesn't
				//exists in server file table anymore
				page_list_free(upcookie->response);
				obj_pool_free(upcookie->owner->file_pool, upcookie);
			} else {
				//Not replied yet, set client_ret to NULL
				//So the server will know
				upcookie->client_req = NULL;
			}
			break;
		default:
			panic("user_close on wrong request\n");
	}

}

int uport_user_available(struct request *req, int rw) {
	if (req->state == REQ_CLOSED)
		return 1;

	if (req->type == REQ_REQUEST) {
		if (rw == 2)
			return req->state == REQ_ESTABLISHED;
		return 0;
	}

	if (req->type != REQ_COOKIE)
		panic("user_available() on wrong type\n");

	struct uport_cookie *upcookie = req->data;
	if (rw == 2)
		return 0; //Can't write to a cookie
	return upcookie->response != NULL;
}

int uport_server_available(struct request *req, int rw) {
	if (rw == 2) {
		if (req->type != REQ_PORT_COOKIE)
			//Can't respond on the port handle
			//Or request handle
			return 0;
		struct uport_cookie *upcookie = req->data;
		if (upcookie->response)
			panic("response already sent but cookie is still here");
		return 1;
	}

	if (req->type == REQ_PORT_COOKIE)
		return 0;
	if (req->type == REQ_PORT) {
		struct uport *up = req->data;
		return !list_empty(&up->incoming);
	}
	if (req->type == REQ_PORT_REQ) {
		struct uport_req *upreq = req->data;
		return !list_empty(&upreq->incoming);
	}
	panic("server_available() on wrong req\n");
	return 0;
}

void uport_server_close(struct request *req) {
	struct uport *up;
	struct uport_req *upreq;
	struct uport_cookie *upcookie;
	struct message *mi, *mnxt;
	struct request *ri, *rnxt;
	switch (req->type) {
		case REQ_PORT:
			up = req->data;
			list_for_each_safe(&up->incoming, mi, mnxt, next) {
				if (mi->plh)
					page_list_free(mi->plh);

				struct task *client_task = mi->client_req->owner;
				mi->client_req->state = REQ_CLOSED;
				if (mi->client_req->waited_rw)
					wake_up(mi->client_req->owner);
				obj_pool_free(client_task->file_pool, mi);
			}
			deregister_port(up->port_number);
			uport[up->port_number] = NULL;
			obj_pool_free(current->file_pool, up);
			break;
		case REQ_PORT_REQ:
			upreq = req->data;
			list_for_each_safe(&upreq->clients, ri, rnxt, next_req) {
				ri->state = REQ_CLOSED;
				list_del(&ri->next_req);
			}
			list_for_each_safe(&upreq->incoming, mi, mnxt, next) {
				if (mi->plh)
					page_list_free(mi->plh);

				struct task *client_task = mi->client_req->owner;
				obj_pool_free(client_task->file_pool, mi);
			}
			obj_pool_free(current->file_pool, upreq);
			break;
		case REQ_PORT_COOKIE:
			upcookie = req->data;
			upcookie->client_req->state = REQ_CLOSED;
			if (upcookie->response)
				panic("cookie already has a response, but it's still in fdtable\n");
			obj_pool_free(current->file_pool, upcookie);
			break;
		default:
			panic("server_close called on wrong type");
			break;
	}
}

struct port_ops uport_pops = {
	.connect = uport_connect,
};

struct req_ops uport_server_rops = {
	.drop = uport_server_close,
	.available = uport_server_available,
};

struct req_ops uport_client_rops = {
	.request = uport_request,
	.get_response = uport_get_response,
	.dup = uport_dup,
	.drop = uport_user_close,
	.available = uport_user_available,
};
