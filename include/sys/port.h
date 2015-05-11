#pragma once
#include <sys/defs.h>
#include <sys/list.h>
#include <uapi/port.h>

enum req_type {
	REQ_COOKIE,
	REQ_REQUEST,
};

struct request {
	enum req_type type;
	bool waited;
	struct list_node next_req;
	void *owner;
	struct req_ops *rops;
	void *data;
};

struct req_ops {
	int (*request)(struct request *req, size_t len, struct list_head *);
	int (*get_response)(struct request *req, struct response *);
	void (*drop_cookie)(struct request *req);
};

struct port_ops {
	int (*connect)(struct request *req, size_t len, struct list_head *);
	void (*drop_connection)(struct request *req);
};

int register_port(int port_number, struct port_ops *);
int queue_response(struct request *req, size_t len, void *buf);
