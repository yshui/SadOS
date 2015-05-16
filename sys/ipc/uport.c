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
	struct list_head consumers;
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

struct port_ops uport_pops;
struct req_ops uport_server_rops;
struct req_ops uport_client_rops;

struct request *uport[MAX_PORT];

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
	struct response res;
	if (req->type == REQ_PORT) {
		struct uport *up = req->data;
		if (list_empty(&up->incoming))
			goto end;
		msg = list_top(&up->incoming, struct message, next);
	} else {
		struct uport_req *upreq = req->data;
		if (!list_empty(&upreq->incoming))
			msg = list_top(&upreq->incoming, struct message, next);
		else if (list_empty(&upreq->consumers)) {
			//All clients are gone
			res.buf = NULL;
			res.len = 0;
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

	uint64_t vaddr = map_to_user(msg->plh, 0)+msg->plh->start_offset;
	if ((long)vaddr < 0) {
		ret = (long)vaddr;
		goto end;
	}

	struct request *creq = msg->client_req;
	res.buf = (void *)vaddr;
	res.len = msg->plh->npage*PAGE_SIZE;
	ret = copy_to_user_simple(&res, buf, sizeof(res));
	if (ret != 0) {
		ret = -EFAULT;
		remove_vma_range(current->as, vaddr, res.len);
		goto end;
	}
	list_del(&msg->next);
	page_list_free(msg->plh);

	if (req->type == REQ_PORT) {
		struct uport_req *upreq = obj_pool_alloc(current->file_pool);
		list_head_init(&upreq->incoming);
		list_head_init(&upreq->consumers);
		list_add(&upreq->consumers, &creq->next_req);
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
	sreq->waited = false;
	creq->state = REQ_ESTABLISHED;

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
	if (!plh)
		goto end;

	current->fds->file[fd] = NULL;
	obj_pool_free(current->file_pool, req);

	struct uport_cookie *upcookie = req->data;
	upcookie->response = plh;
	if (upcookie->client_req->waited)
		wake_up(upcookie->client_req->owner);
end:
	enable_interrupts();
	return ret;
}

int uport_connect(int port, struct request *req, size_t len, void *buf) {
	struct page_list_head *plh = map_from_user(buf, len);
	struct message *msg = obj_pool_alloc(plh->pg);
	msg->plh = plh;
	msg->client_req = req;

	struct uport *up = uport[port]->data;
	req->data = NULL;
	req->rops = &uport_client_rops;
	req->state = REQ_PENDING;
	list_add_tail(&up->incoming, &msg->next);
	return 0;
}

int uport_request(struct request *req, size_t len, void *buf) {
	if (req->data == (void *)1)
		//Server side has closed connection
		return -EPIPE;
	if (req->state == REQ_PENDING)
		//Server side has not acknowledged this connection
		return -EAGAIN;

	struct request *sreq = req->data;
	struct page_list_head *plh = map_from_user(buf, len);
	if (!plh)
		return -EFAULT;
	struct message *msg = obj_pool_alloc(plh->pg);
	msg->client_req = req;
	msg->plh = plh;
	req->data = msg;
	req->state = REQ_PENDING;

	struct uport_req *upreq = sreq->data;
	list_add_tail(&upreq->incoming, &msg->next);

	if (sreq->waited)
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
	list_add(&upreq->consumers, &new_req->next_req);
	return 0;
}

void uport_user_close(struct request *req) {
	if (req->state == REQ_PENDING) {
		struct message *msg = req->data;
		list_del(&msg->next);
		page_list_free(msg->plh);
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

int uport_user_available(struct request *req) {
	if (req->type != REQ_COOKIE)
		return 0;
	struct uport_cookie *upcookie = req->data;
	return !upcookie->response;
}

int uport_server_available(struct request *req) {
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
			list_for_each_safe(&up->incoming, mi, mnxt, next)
				page_list_free(mi->plh);
			deregister_port(up->port_number);
			uport[up->port_number] = NULL;
			obj_pool_free(current->file_pool, up);
			break;
		case REQ_PORT_REQ:
			upreq = req->data;
			list_for_each_safe(&upreq->consumers, ri, rnxt, next_req) {
				ri->data = (void *)1; //The server has closed connection
				list_del(&ri->next_req);
			}
			list_for_each_safe(&upreq->incoming, mi, mnxt, next)
				page_list_free(mi->plh);
			obj_pool_free(current->file_pool, upreq);
			break;
		case REQ_PORT_COOKIE:
			upcookie = req->data;
			upcookie->client_req->data = (void *)1;
			page_list_free(upcookie->response);
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
