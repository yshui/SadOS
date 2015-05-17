#pragma once
#include <sys/defs.h>

struct portio_req {
	uint32_t data;
	uint16_t port;
	uint8_t type;
	uint8_t len;
};
