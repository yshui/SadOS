#include <uapi/port.h>

enum io_type {
	IO_READ,
	IO_WRITE,
};

struct io_req {
	enum io_type type;
	size_t len;
};

struct io_res {
	int err;
	size_t len;
};
