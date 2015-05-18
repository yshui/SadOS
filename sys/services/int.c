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
#include <sys/list.h>
#include <sys/port.h>
#include <sys/service.h>
#include <sys/copy.h>

struct list_head int_wait_list[256];

struct port_ops int_pops;
struct req_ops int_rops;


void int_port_init(void) {
	for (int i = 0; i < 256; i++)
		list_head_init(&int_wait_list[i]);
	register_port(10, &int_pops);
}
SERVICE_INIT(int_port_init, int_port);

int int_port_connect(int port, struct request *req, size_t len, void *buf) {
	int int_number;
	if (len != sizeof(int))
		return -EINVAL;
	int ret = copy_from_user_simple(buf, &int_number, sizeof(int_number));
	if (ret)
		return -EFAULT;
	if (int_number < 33 || int_number > 255)
		return -EINVAL;
	req->data = (void *)(uint64_t)int_number;
	req->rops = &int_rops;
	list_add(&int_wait_list[int_number], &req->next_req);
	return 0;
}

int int_port_available(struct request *req, int rw) {
	if (rw == 2)
		return 0;
	uint64_t x = (uint64_t)req->data;
	int ret = (x&256) != 0;
	req->data = (void *)(x&255);
	return ret;
}

int int_port_dup(struct request *old, struct request *new) {
	int int_number = (uint64_t)old->data&255;
	new->data = (void *)(uint64_t)int_number;
	new->rops = &int_rops;

	list_add(&int_wait_list[int_number], &new->next_req);
	return 0;
}

void int_port_drop(struct request *req) {
	list_del(&req->next_req);
}

struct port_ops int_pops = {
	.connect = int_port_connect,
};

struct req_ops int_rops = {
	.dup = int_port_dup,
	.available = int_port_available,
	.drop = int_port_drop,
};
