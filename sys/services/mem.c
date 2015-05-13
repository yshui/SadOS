#include <sys/port.h>
#include <uapi/mem.h>
#include <sys/copy.h>
#include <sys/service.h>
#include <sys/printk.h>
#include <sys/interrupt.h>
#include <sys/mm.h>
#include <errno.h>
struct port_ops mem_pops;
struct req_ops mem_rops;
struct mem_data {
	struct obj_pool *obp;
	void *buf;
	size_t len;
};
int mem_port_connect(struct request *req, size_t len, struct list_head *pgs) {
	if (len != sizeof(struct mem_req))
		return -EINVAL;

	//pgs should have only one page
	struct mem_req *mreq = list_top(pgs, struct page_list, next)->page;
	if (!mreq)
		//User send a zero page
		//Which is invalid
		return -EINVAL;
	struct obj_pool *op = obj_pool_create(sizeof(struct mem_data));
	disable_interrupts();
	struct mem_data *md = obj_pool_alloc(op);
	md->obp = op;
	struct vm_area *vma;
	uint64_t alen = ALIGN_UP(len, PAGE_SIZE_BIT);
	long ret = 0, ret2;

	int vma_flags = VMA_RW;
	if (mreq->type == MAP_PHYSICAL) {
		printk("Client asking for %p -> %p, len %d\n", mreq->phys_addr, mreq->dest_addr, mreq->len);
		vma_flags |= VMA_HW;
	} else
		printk("Client asking for %p(ignored) -> %p, len %d\n", mreq->phys_addr, mreq->dest_addr, mreq->len);

	if (mreq->dest_addr)
		vma = insert_vma_to_vaddr(current->as, mreq->dest_addr, alen, vma_flags);
	else
		vma = insert_vma_to_any(current->as, alen, vma_flags);
	if (PTR_IS_ERR(vma)) {
		printk("Can't insert vma, return error\n");
		ret = -ENOMEM;
		goto end;
	}
	printk("Actually mapped at %p, len %d\n", vma->vma_begin, alen);

	switch (mreq->type) {
		case MAP_PHYSICAL:
			if (!IS_ALIGNED(mreq->phys_addr, PAGE_SIZE_BIT)) {
				ret = -EINVAL;
				goto end;
			}
			ret2 = insert_range(mreq->phys_addr, alen);
			if (ret2 < 0) {
				ret = -EINVAL;
				goto end;
			}
			ret2 = address_space_assign_addr_range(current->as, mreq->phys_addr, vma->vma_begin, alen);
			if (ret2 < 0)
				panic("Impossible error\n");
			md->buf = (void *)vma->vma_begin;
			md->len = alen;
			break;
		case MAP_NORMAL:
			md->buf = (void *)vma->vma_begin;
			md->len = alen;
			break;
		default:
			obj_pool_destroy(op);
			ret = -EINVAL;
			goto end;
	}
	req->data = md;
	req->rops = &mem_rops;

end:
	enable_interrupts();
	printk("Ret=%d", ret);
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
void mem_service_init(void){
	printk("Memory port init\n");
	register_port(1, &mem_pops);
}
SERVICE_INIT(mem_service_init, mem_port);
struct port_ops mem_pops = {
	.connect = mem_port_connect,
};
struct req_ops mem_rops = {
	.request = mem_port_connect,
	.get_response = mem_port_get_response,
};
