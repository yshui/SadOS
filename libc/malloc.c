#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "brk.h"
//we will align allocation to 8 bytes
static void *last_brk = NULL;
void *malloc(size_t s){
	size_t newsize = s+8; //8 byte for header
	newsize = (newsize+7)&((size_t)-8);

	void *oldbrk = sbrk(newsize);
	if (oldbrk == (void *)-1)
		return NULL;

	//set last bit to mark memory as allocated
	*(size_t *)oldbrk = ((uint64_t)last_brk)|1;
	last_brk = oldbrk;
	return ((size_t *)oldbrk)+1;
}
void free(void *ptr){
	if (!ptr)
		return;
	size_t *hdr = (((size_t *)ptr)-1);
	size_t size = *hdr;
	if (!size&1) {
		printf("Invalid free()\n");
		exit(1);
	}
	*hdr = size^1;
	if (hdr == last_brk) {
		//try to free as much memory as possible
		size_t *tmp = (size_t *)*hdr;
		size_t *tmpn = hdr;
		while (tmp && !(*tmp)&1) {
			tmpn = tmp;
			tmp = *(size_t **)tmp;
		}
		if (brk(tmpn) < 0) {
			printf("Failed to *free* memory???\n");
			exit(1);
		}
		last_brk = tmpn;
	}
}
