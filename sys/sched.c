/* Copyright (C) 2015, Yuxuan Shui <yshuiv7@gmail.com> */

/*
 * Everyone is allowed to copy and/or redistribute verbatim
 * copies of this code as long as the following conditions
 * are met:
 *
 * 1. The copyright infomation and license is unchanged.
 *
 * 2. This piece of code is not submitted as or as part of
 *    an assignment to the CSE506 course at Stony Brook
 *    University, unless the submitter is the copyright
 *    holder.
 */
#include <sys/list.h>
#include <sys/ipc.h>
#include <sys/sched.h>
#include <sys/mm.h>
#include <sys/msr.h>
#include <sys/interrupt.h>
#include <sys/port.h>
#include <sys/copy.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/list.h>
struct list_head runq;
struct task *tasks[32767], *idle_task;
static struct obj_pool *task_pool = NULL;
struct task *current, *to_kill;
uint64_t kernel_stack;
int current_pid;
extern void switch_to(struct task *);
static void kill_process(struct task *t) {
	if (t == current)
		panic("Trying to kill current processs");
	destroy_as(t->as);
	obj_pool_destroy(t->file_pool);
	drop_page(t->fds);
	drop_page(t->kstack_base-PAGE_SIZE);

	struct task *child, *tmp;
	list_for_each_safe(&t->children, child, tmp, siblings) {
		list_del(&child->siblings);
		child->parent = NULL;
	}

	if (!t->parent) {
		tasks[t->pid] = NULL;
		obj_pool_free(task_pool, t);
	}
}

void after_switch(void) {
	current->state = TASK_RUNNING;
	if (to_kill) {
		kill_process(to_kill);
		to_kill = NULL;
	}
	enable_interrupts();
}

static inline void load_cr3(struct address_space *as) {
	uint64_t cr3 = ((uint64_t)as->pml4)-KERN_VMBASE;
	uint64_t *new_pml4 = as->pml4;
	//Update kernel mappings in the pml4
	uint64_t *pml4hh = get_pte_addr(0x101ull<<39, 0);
	memcpy(new_pml4+257, pml4hh, 255*8);
	__asm__ volatile ("movq %0, %%cr3" : : "r"(cr3));
}
void schedule(void) {
	//FIXME Implement the simple O(1) scheduler
	if (interrupt_disabled)
		panic("schedule() called with disabled interrupt\n");
	if (to_kill)
		panic("Why is to_kill set????");
	disable_interrupts();
	if (current->state == TASK_RUNNING) {
		current->state = TASK_RUNNABLE;
		list_add_tail(&runq, &current->tasks);
	}
	struct task *next;
	if (!list_empty(&runq)) {
		next = list_top(&runq, struct task, tasks);
		list_del(&next->tasks);
	} else
		next = idle_task;
	printk("%p\n", next);

	printk("Next: %d, stack: %p\n", next->pid, next->krsp);
	current_pid = next->pid;

	//Switch cr3

	//next->as == NULL means kernel task
	//Don't need to change cr3
	if (next->as)
		load_cr3(next->as);
	kernel_stack = (uint64_t)next->kstack_base;

	if (current->state == TASK_ZOMBIE)
		to_kill = current;
	//Save current stack pointer
	switch_to(next);
	after_switch();
	return;
}
void int_reschedule(uint8_t *sp) {
	//This function is called with interrupt disabled
	//Switch from interrupt stack to per-task thread
	//And copy saved registers
	register uint8_t *kstack = current->kstack_base;
	kstack -= 5*8;  //Interrupt stack frame
	kstack -= 15*8; //15 registers
	kstack -= 8;    //Return address
	if (sp < current->kstack_base-PAGE_SIZE || sp > current->kstack_base) {
		printk("copy to kernel stack");
		memcpy(kstack, sp-8, (5+15+1)*8);
	} else {
		//We are already on kernel stack
		printk("already on kernel stack");
		kstack = sp-8;
	}
	interrupt_disabled = 0;

	//Switch to kernel stack
	__asm__ volatile ("\tmovq %0, %%rsp\n"
			  "\tsti\n" //Enable interrupt because we are now on safe stack
			  "\tcall schedule\n"
			  "\tret\n" : : "S"(kstack) :
			 );
	__builtin_unreachable();
}
struct task *new_task(void) {
	return obj_pool_alloc(task_pool);
}
extern char ret_new_process, new_task_return;
struct task *new_process(struct address_space *as, struct thread_info *ti) {
	struct task *new_task = obj_pool_alloc(task_pool);
	void *stack_page = get_page();
	memset(stack_page, 0, PAGE_SIZE);

	new_task->kstack_base = stack_page+PAGE_SIZE;
	new_task->as = as;
	as->owner = new_task;
	new_task->state = TASK_RUNNABLE;
	new_task->file_pool = obj_pool_create(sizeof(struct request));
	new_task->fds = fdtable_new();
	new_task->astable = fdtable_new();
	new_task->astable->file[0] = as;
	new_task->ti = NULL;
	list_head_init(&new_task->children);

	//We need to forge the kernel stack of this process
	//So when we schedule(), we can switch to this task

	uint8_t *x = (void *)new_task->kstack_base;
	ti->r11 |= BIT(9);
	memcpy(x-sizeof(*ti), ti, sizeof(*ti));

	uint64_t *sb = (void *)(x-sizeof(*ti));
	*(--sb) = (uint64_t)&new_task_return; //The schedule() return address
	*(--sb) = (uint64_t)&after_switch;

	//Move up 6 move qword, for the registers saved by switch_to
	sb -= 6;
	memset(sb, 0, 6*sizeof(uint64_t));
	new_task->krsp = (void *)sb;
	return new_task;
}

void task_init(void) {
	task_pool = obj_pool_create(sizeof(struct task));
	list_head_init(&runq);

	//Enable syscall/sysret
	uint64_t efer = rdmsr(MSR_EFER);
	efer |= 1;
	wrmsr(MSR_EFER, efer);
}

void wake_up(struct task *t) {
	if (t->state == TASK_WAITING) {
		t->state = TASK_RUNNABLE;
		list_add(&runq, &t->tasks);
	}
}

static int available_task_slot(void) {
	int i;
	for (i = 0; i < 32767; i++)
		if (!tasks[i])
			return i;
	return -1;
}

SYSCALL(1, exit, int, code) {
	kill_current(code);
	schedule();
	__builtin_unreachable();
}

SYSCALL(1, get_thread_info, void *, buf) {
	copy_to_user_simple(current->ti, buf, sizeof(struct thread_info));
	return 0;
}

//Create a new task with address space as, and thread_info buf
SYSCALL(3, create_task, int, as, void *, buf, int, flags) {
	//0 is this task's address space
	if (as <= 0 || as > current->astable->max_fds || !current->astable->file[as])
		return -EBADF;
	disable_interrupts();
	struct thread_info ti;
	if (copy_from_user_simple(buf, &ti, sizeof(ti)) != 0) {
		enable_interrupts();
		return -EINVAL;
	}

	//Remove 'as' from task's file table
	struct address_space *_as = current->astable->file[as];

	if (flags & CT_SELF) {
		destroy_as(current->as);
		current->astable->file[as] = NULL;
		ti.r11 |= BIT(9); //Set IF
		memcpy(current->ti, &ti, sizeof(ti));
		current->as = _as;
		current->astable->file[0] = _as;
		load_cr3(_as);
		enable_interrupts();
		return 0;
	}

	int pid = available_task_slot();
	if (pid < 0) {
		enable_interrupts();
		return -ENOMEM;
	}

	current->astable->file[as] = NULL;

	struct task *ntask = new_process(_as, &ti);

	//Copy fdtable
	for (int i = 0; i < current->fds->max_fds; i++) {
		if (!current->fds->file[i])
			continue;

		struct request *req = current->fds->file[i];
		if (req->type == REQ_COOKIE)
			continue;

		if (!req->rops->dup)
			continue;

		struct request *new_req = obj_pool_alloc(ntask->file_pool);
		new_req->owner = ntask;
		new_req->waited_rw = 0;
		new_req->type = req->type;
		if (req->rops->dup(req, new_req) != 0) {
			obj_pool_free(ntask->file_pool, new_req);
			continue;
		}

		ntask->fds->file[i] = new_req;
	}
	ntask->priority = current->priority;
	ntask->state = TASK_RUNNABLE;
	ntask->pid = pid;
	ntask->parent = current;
	tasks[pid] = ntask;
	list_add(&runq, &ntask->tasks);
	list_add(&current->children, &ntask->siblings);
	enable_interrupts();
	return pid;
}

SYSCALL(0, getpid) {
	return current->pid;
}

SYSCALL(0, getppid) {
	disable_interrupts();

	int ret = 0;
	if (current->parent)
		ret = current->parent->pid;
	enable_interrupts();
	return ret;
}
