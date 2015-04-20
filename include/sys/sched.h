#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/list.h>
struct task {
	struct list_head tasks;
	uint64_t regs[16];
	uint64_t rip;
	uint8_t *kern_stack;
	struct address_space *as;
	uint32_t state;
	uint32_t pid;
};

extern struct task *current;
