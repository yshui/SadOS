#pragma once
#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/list.h>
#include <sys/ipc.h>

enum task_state {
	TASK_RUNNABLE = 0,
	TASK_RUNNING,
	TASK_WAITING,
};

struct task {
	uint8_t *kstack_base; //Upon entry from user space, stack_base is used
	uint8_t *krsp; //When switching task in kernel, saved rsp is used
	int state;
	struct list_node tasks;
	struct fdtable *fds;
	struct obj_pool *file_pool;
	struct address_space *as;
	uint32_t pid;
	int priority;
};

extern struct task *current;
extern struct list_head tasks;

extern char syscall_return;

struct task *new_task(void);
void schedule(void);
struct task *new_process(struct address_space *as, uint64_t rip, uint64_t stack_base);
void wake_up(struct task *t);
void task_init(void);
void kill_current(void);
