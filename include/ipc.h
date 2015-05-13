#pragma once
#include <uapi/port.h>

long port_connect(int, size_t, void *);
long get_response(int, struct response *);
long request(int, size_t, void *);
