#include <sys/defs.h>
struct mem_req {
	uint64_t phys_addr, dest_addr;
	size_t len;
};
