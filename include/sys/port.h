#pragma once
#include <sys/defs.h>
#include <sys/list.h>
#include <uapi/port.h>

#define MAX_PORT 4096

enum req_type {
	REQ_COOKIE,
	REQ_REQUEST,
	REQ_PORT,
	REQ_PORT_REQ,
	REQ_PORT_COOKIE,
};

enum req_state {
	REQ_SERVER,
	REQ_PENDING,
	REQ_ESTABLISHED,
	REQ_CLOSED,
};

struct request {
	struct list_node next_req;
	void *owner;
	struct req_ops *rops;
	void *data;
	enum req_type type;
	enum req_state state;
	uint8_t waited_rw;
};

struct req_ops {
	int (*request)(struct request *req, struct request *cookie, size_t len, void *buf);
	int (*get_response)(struct request *req, struct response *);
	void (*drop)(struct request *req);
	int (*dup)(struct request *old_req, struct request *new_req);
	int (*available)(struct request *req, int rw);
};

struct port_ops {
	int (*connect)(int port, struct request *req, size_t len, void *buf);
};

int register_port(int port_number, struct port_ops *);
void deregister_port(int port_number);
int queue_response(struct request *req, size_t len, void *buf);
void port_init(void);
