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
#include <sys/port.h>
#include <uapi/mem.h>
#include <sys/copy.h>
#include <sys/service.h>
#include <sys/printk.h>
#include <sys/interrupt.h>
#include <sys/mm.h>
#include <sys/aspace.h>
#include <errno.h>
struct port_ops mem_pops;
struct req_ops mem_rops;
struct mem_data {
	struct obj_pool *obp;
	void *buf;
	size_t len;
};
int mem_port_request(struct request *req, struct request *reqc, size_t len, void *buf) {
	if (len != sizeof(struct mem_req))
		return -EINVAL;

	struct mem_req mreq;
	if (!buf || len != sizeof(mreq))
		return -EINVAL;

	disable_interrupts();

	long ret = copy_from_user_simple(buf, &mreq, sizeof(mreq));
	if (ret != 0)
		goto end;

	ret = -EINVAL;
	if (!IS_ALIGNED(mreq.phys_addr, PAGE_SIZE_BIT))
		goto end;

	uint64_t alen = ALIGN_UP(len, PAGE_SIZE_BIT);
	struct vm_area *vma;

	printk("Client asking for %p -> %p, len %d\n", mreq.phys_addr, mreq.dest_addr, mreq.len);

	if (mreq.dest_addr)
		vma = insert_vma_to_vaddr(current->as, mreq.dest_addr, alen, VMA_RW|VMA_HW);
	else
		vma = insert_vma_to_any(current->as, alen, VMA_RW|VMA_HW);

	ret = -ENOMEM;
	if (PTR_IS_ERR(vma)) {
		printk("Can't insert vma, return error\n");
		goto end;
	}

	printk("Actually mapped at %p, len %d\n", vma->vma_begin, alen);

	ret = -EINVAL;
	if (insert_range(mreq.phys_addr, alen) < 0) {
		*vma->prev = vma->next;
		if (vma->next)
			vma->next->prev = vma->prev;
		obj_pool_free(current->as->vma_pool, vma);
		goto end;
	}

	if (address_space_assign_addr_range(current->as, mreq.phys_addr, vma->vma_begin, alen) < 0)
		panic("Impossible error\n");

	struct obj_pool *op = obj_pool_create(sizeof(struct mem_data));
	struct mem_data *md = obj_pool_alloc(op);

	md->obp = op;
	md->buf = (void *)vma->vma_begin;
	md->len = alen;
	reqc->data = md;
	reqc->rops = &mem_rops;
	ret = 0;
end:
	enable_interrupts();
	printk("Ret=%d\n", ret);
	return ret;
}

int mem_port_get_response(struct request *req, struct response *res) {
	disable_interrupts();
	struct mem_data *md = req->data;
	req->data = NULL;
	enable_interrupts();

	if (!md)
		return -ENOMEM;
	res->buf = md->buf;
	res->len = md->len;
	obj_pool_destroy(md->obp);
	return 0;
}

void mem_port_close(struct request *req) {
	if (!req->data)
		return;
	struct mem_data *md = req->data;
	obj_pool_destroy(md->obp);
}
int mem_port_connect(int port, struct request *req, size_t len, void *buf) {
	return mem_port_request(NULL, req, len, buf);
}
void mem_service_init(void){
	printk("Memory port init\n");
	register_port(1, &mem_pops);
}
SERVICE_INIT(mem_service_init, mem_port);
struct port_ops mem_pops = {
	.connect = mem_port_connect,
};
struct req_ops mem_rops = {
	.request = mem_port_request,
	.get_response = mem_port_get_response,
	.drop = mem_port_close,
};
