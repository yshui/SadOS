#include <sys/port.h>
#include <sys/portio.h>
#include <sys/service.h>
#include <uapi/portio.h>
#include <sys/copy.h>
#include <sys/interrupt.h>

struct port_ops portio_pops;
struct req_ops portio_rops;


void portio_init(void) {
	register_port(4, &portio_pops);
}
SERVICE_INIT(portio_init, portio_port);

int portio_connect(int port, struct request *req, size_t len, void *buf) {
	req->data = NULL;
	req->rops = &portio_rops;
	return 0;
}

int portio_request(struct request *req, struct request *reqc, size_t len, void *buf) {
	struct portio_req preq;
	if (len != sizeof(preq))
		return -EINVAL;
	int ret = copy_from_user_simple(buf, &preq, sizeof(preq));
	if (ret != 0)
		return -EFAULT;
	reqc->rops = &portio_rops;
	if (preq.type == 1) {
		printk("Writing %p to port %p\n", preq.data, preq.port);
		if (preq.len == 1)
			outb(preq.port, preq.data);
		else if (preq.len == 4)
			outl(preq.port, preq.data);
		reqc->data = (void *)-1;
		return 0;
	} else if (preq.type == 2) {
		if (preq.len == 1)
			reqc->data = (void *)(uint64_t)inb(preq.port);
		else
			reqc->data = (void *)(uint64_t)inl(preq.port);
		printk("Read %p from port %p\n", reqc->data, preq.port);
		return 0;
	}
	return -EINVAL;
}

int portio_get_response(struct request *req, struct response *res) {
	if (req->type != REQ_COOKIE)
		return -EBADF;
	if ((long)req->data < 0)
		return -EINVAL;
	res->buf = req->data;
	return 0;
}

struct port_ops portio_pops = {
	.connect = portio_connect,
};

struct req_ops portio_rops = {
	.request = portio_request,
	.get_response = portio_get_response,
};
