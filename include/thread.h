#pragma once
#include <uapi/thread.h>

long get_thread_info(struct thread_info *);
long create_task(int as, struct thread_info *, int flags);
