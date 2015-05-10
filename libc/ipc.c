#include <syscall.h>
#include <uapi/port.h>
#include "simple_syscalls.h"

sys3(port_connect, int, size_t, void *, long)
sys2(get_response, int, struct response *, long)
