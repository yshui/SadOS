#include <syscall.h>
#include <uapi/port.h>
#include "simple_syscalls.h"

sys3(port_connect, int, size_t, void *, long)
sys2(get_response, int, struct response *, long)
sys3(request, int, size_t, void *, long)
sys1(open_port, int, long)
sys1(wait_on_port, int, long)
sys3(wait_on, struct fd_set *, struct fd_set *, int, long)
sys2(pop_request, int, struct response *, long)
