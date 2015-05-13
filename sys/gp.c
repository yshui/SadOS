#include <sys/defs.h>
#include <sys/panic.h>
#include <sys/sched.h>
int gp_handler(uint64_t *rsp) {
	uint64_t saved_rip = *(rsp+1);
	if (((long)saved_rip) < 0)
		panic("GP in kernel, %p\n", saved_rip);
	printk("User space program GP, %p\n", saved_rip);
	kill_current(2);
	return 1;
}
