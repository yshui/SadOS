#include <sys/list.h>
#include <sys/ipc.h>
#include <sys/sched.h>
#include <sys/mm.h>
#include <sys/msr.h>
#include <sys/interrupt.h>
#include <sys/port.h>
#include <string.h>
struct list_head tasks;
static struct obj_pool *task_pool = NULL;
struct task *current;
uint64_t kernel_stack;
extern void switch_to(struct task *);
void schedule() {
	//FIXME Implement the simple O(1) scheduler
	register struct task *t, *next = NULL;
	list_for_each(&tasks, t, tasks)
		if (t->state == RUNNABLE &&
		    (!next || t->priority > next->priority))
			next = t;

	//Switch cr3

	//next->as == NULL means kernel task
	//Don't need to change cr3
	if (next->as) {
		uint64_t cr3 = ((uint64_t)next->as->pml4)-KERN_VMBASE;
		uint64_t *new_pml4 = next->as->pml4;
		//Update kernel mappings in the pml4
		uint64_t *pml4hh = get_pte_addr(0x101ull<<39, 0);
		memcpy(new_pml4+257, pml4hh, PAGE_SIZE/2);
		__asm__ volatile ("movq %0, %%cr3" : : "r"(cr3));
	}
	kernel_stack = (uint64_t)next->kstack_base;

	current->state = RUNNABLE;
	//Save current stack pointer
	switch_to(next);
	current->state = RUNNING;
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
struct task *new_process(struct address_space *as,
			 uint64_t rip, uint64_t stack_base) {
	struct task *new_task = obj_pool_alloc(task_pool);
	void *stack_page = get_page();
	memset(stack_page, 0, PAGE_SIZE);

	new_task->kstack_base = stack_page+PAGE_SIZE;
	new_task->as = as;
	new_task->state = RUNNABLE;
	new_task->file_pool = obj_pool_create(sizeof(struct request));
	new_task->fds = get_page();
	memset(new_task->fds, 0, PAGE_SIZE);
	new_task->fds->file = (void *)(((uint8_t *)new_task->fds)+sizeof(struct fdtable));
	new_task->fds->max_fds = (PAGE_SIZE-sizeof(struct fdtable))/8;

	//We need to forge the kernel stack of this process
	//So when we schedule(), we can switch to this task

	uint64_t *sb = (void *)new_task->kstack_base;
	*(--sb) = stack_base;
	sb -= 3;
	*(--sb) = rip; //RCX

	sb -= 6;
	*(--sb) = BIT(9); //R11 is rflags, set IF

	sb -= 4;
	*(--sb) = (uint64_t)&syscall_return; //The schedule() return address

	//Move up 6 move qword, for the registers saved by switch_to
	sb -= 6;
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
	t->state = RUNNABLE;
}
