#include <uapi/port.h>

enum io_type {
	IO_READ,
	IO_WRITE,
    IO_OPEN,
    IO_OPENDIR,
    IO_READDIR,
    IO_LSEEK
};

struct io_req {
	int type;
	int flags;
	size_t len;
};

struct io_res {
	int err;
	size_t len;
};
