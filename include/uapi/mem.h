#include <sys/defs.h>
struct mem_req {
	uint64_t phys_addr;
	size_t len;
	enum {MAP_PHYSICAL, MAP_NORMAL} type;
};
