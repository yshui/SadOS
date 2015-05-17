#pragma once
#include <uapi/port.h>

long port_connect(int, size_t, void *);
long get_response(int, struct response *);
long request(int, size_t, void *);
long open_port(int);
long wait_on(struct fd_set *rfds, struct fd_set *wfds, int timeout);
long pop_request(int pd, struct urequest *);
long wait_on_port(int port_number);
long respond(int, size_t, void *);
