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
struct list_head tasks;
static struct obj_pool *task_pool = NULL;
struct task *current, *to_kill;
uint64_t kernel_stack;
extern void switch_to(struct task *);
static void kill_process(struct task *t) {
	if (t == current)
		panic("Trying to kill current processs");
	destroy_as(t->as);
	obj_pool_destroy(t->file_pool);
	drop_page(t->fds);
	drop_page(t->kstack_base-PAGE_SIZE);
}

void after_switch(void) {
	current->state = TASK_RUNNING;
	if (to_kill) {
		kill_process(to_kill);
		to_kill = NULL;
	}
	enable_interrupts();
}
void schedule(void) {
	//FIXME Implement the simple O(1) scheduler
	if (interrupt_disabled)
		panic("schedule() called with disabled interrupt\n");
	if (to_kill)
		panic("Why is to_kill set????");
	disable_interrupts();
	register struct task *t, *next = NULL;
	list_for_each(&tasks, t, tasks)
		if (t->state == TASK_RUNNABLE &&
		    (!next || t->priority > next->priority))
			next = t;

	printk("Next: %d, stack: %p\n", next->pid, next->krsp);

	//Switch cr3

	//next->as == NULL means kernel task
	//Don't need to change cr3
	if (next->as) {
		uint64_t cr3 = ((uint64_t)next->as->pml4)-KERN_VMBASE;
		uint64_t *new_pml4 = next->as->pml4;
		//Update kernel mappings in the pml4
		uint64_t *pml4hh = get_pte_addr(0x101ull<<39, 0);
		memcpy(new_pml4+257, pml4hh, 255*8);
		__asm__ volatile ("movq %0, %%cr3" : : "r"(cr3));
	}
	kernel_stack = (uint64_t)next->kstack_base;

	if (current->state == TASK_RUNNING)
		current->state = TASK_RUNNABLE;
	else if (current->state == TASK_ZOMBIE)
		to_kill = current;
	//Save current stack pointer
	switch_to(next);
	after_switch();
	return;
}
void int_reschedule(char *sp) {
	//This function is called with interrupt disabled
	//Switch from interrupt stack to per-task thread
	//And copy saved registers
	register uint8_t *kstack = current->kstack_base;
	kstack -= 5*8;  //Interrupt stack frame
	kstack -= 15*8; //15 registers
	kstack -= 8;    //Return address
	memcpy(kstack, sp, (5+15+1)*8);
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
extern char ret_new_process;
struct task *new_process(struct address_space *as, struct thread_info *ti) {
	struct task *new_task = obj_pool_alloc(task_pool);
	void *stack_page = get_page();
	memset(stack_page, 0, PAGE_SIZE);

	new_task->kstack_base = stack_page+PAGE_SIZE;
	new_task->as = as;
	new_task->state = TASK_RUNNABLE;
	new_task->file_pool = obj_pool_create(sizeof(struct request));
	new_task->fds = fdtable_new();
	new_task->astable = fdtable_new();
	new_task->ti = NULL;

	//We need to forge the kernel stack of this process
	//So when we schedule(), we can switch to this task

	uint8_t *x = (void *)new_task->kstack_base;
	ti->r11 |= BIT(9);
	memcpy(x-sizeof(*ti), ti, sizeof(*ti));

	uint64_t *sb = (void *)(x-sizeof(*ti));
	*(--sb) = (uint64_t)&syscall_return; //The schedule() return address
	*(--sb) = (uint64_t)&after_switch;

	//Move up 6 move qword, for the registers saved by switch_to
	sb -= 6;
	memset(sb, 0, 6*sizeof(uint64_t));
	new_task->krsp = (void *)sb;
	return new_task;
}

void task_init(void) {
	list_head_init(&tasks);
	task_pool = obj_pool_create(sizeof(struct task));

	//Enable syscall/sysret
	uint64_t efer = rdmsr(MSR_EFER);
	efer |= 1;
	wrmsr(MSR_EFER, efer);
}

void wake_up(struct task *t) {
	t->state = TASK_RUNNABLE;
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
	if (as < 0 || as > current->astable->max_fds || !current->astable->file[as])
		return -EBADF;
	struct thread_info ti;
	if (copy_from_user_simple(buf, &ti, sizeof(ti)) != 0)
		return -EINVAL;

	//Remove 'as' from task's file table
	struct address_space *_as = current->astable->file[as];
	current->fds->file[as] = NULL;

	struct task *ntask = new_process(_as, &ti);

	//Copy fdtable
	for (int i = 0; i < current->fds->max_fds; i++) {
		if (!current->fds->file[i])
			continue;
		struct request *req = current->fds->file[i];
		if (req->type == REQ_COOKIE)
			continue;
		ntask->fds->file[i] = req;
	}
	ntask->priority = current->priority;
	ntask->state = TASK_RUNNABLE;
	list_add(&tasks, &ntask->tasks);
	return 0;
}
