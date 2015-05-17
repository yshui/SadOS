#pragma once
#include <sys/defs.h>

struct portio_req {
	uint16_t port;
	uint8_t byte;
	uint8_t type;
};
