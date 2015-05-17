#pragma once
#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/list.h>
#include <sys/ipc.h>
#include <uapi/thread.h>

#define __SCHED_H__

enum task_state {
	TASK_RUNNABLE = 0,
	TASK_RUNNING,
	TASK_WAITING,
	TASK_ZOMBIE,
};

struct task {
	uint8_t *kstack_base; //Upon entry from user space, stack_base is used
	uint8_t *krsp; //When switching task in kernel, saved rsp is used
	struct thread_info *ti;
	struct task *parent;
	struct list_head children;
	int state;
	struct list_node tasks;
	struct list_node siblings;
	struct fdtable *fds;
	struct fdtable *astable;
	struct obj_pool *file_pool;
	struct address_space *as;
	uint32_t pid;
	uint32_t exit_code;
	int priority;
};

extern struct task *current, *idle_task;
extern struct list_head runq;
extern struct task *tasks[];

extern char syscall_return;

struct task *new_task(void);
void schedule(void);
struct task *new_process(struct address_space *as, struct thread_info *ti);
void wake_up(struct task *t);
void task_init(void);
static inline void kill_current(int exit_code) {
	current->state = TASK_ZOMBIE;
	current->exit_code = exit_code;
}
