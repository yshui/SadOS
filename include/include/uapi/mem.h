#include <sys/defs.h>
enum map_type {
	MAP_PHYSICAL,
	MAP_NORMAL,
};
struct mem_req {
	uint64_t phys_addr, dest_addr;
	size_t len;
	enum map_type type;
};
