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
#include <sys/ipc.h>
#include <sys/defs.h>
#include <sys/port.h>
#include <sys/tarfs.h>
#include <sys/sched.h>
#include <sys/service.h>
#include <sys/kernaddr.h>
struct req_ops bda_rops;
struct port_ops bda_pops;
void bda_port_init(void) {
	register_port(3, &bda_pops);

	for (int i = 0; i < PAGE_SIZE*16; i+=PAGE_SIZE)
		manage_phys_page(i);
}
SERVICE_INIT(bda_port_init, bda_port);

int bda_port_connect(int port, struct request *req, size_t len, void *buf) {
	req->data = NULL;
	req->rops = &bda_rops;
	return 0;
}
int bda_port_get_response(struct request *req, struct response *res) {
	uint64_t len = PAGE_SIZE*16;

	struct vm_area *vma = insert_vma_to_any(current->as, len, VMA_RW);
	uint64_t vaddr = vma->vma_begin;
	for (int i = 0; i < PAGE_SIZE*16; i += PAGE_SIZE) {
		struct page *p = manage_phys_page(i);
		address_space_assign_page(current->as, p, vaddr, PF_SNAPSHOT);
		page_unref(p, 0); //This page has a ref_count from bda_init
		vaddr += PAGE_SIZE;
	}
	res->buf = (void *)vma->vma_begin;
	res->len = len;
	return 0;
}
struct port_ops bda_pops = {
	.connect = bda_port_connect,
};
struct req_ops bda_rops = {
	.get_response = bda_port_get_response,
};
