#include <sys/ipc.h>
#include <sys/defs.h>
#include <sys/port.h>
#include <sys/tarfs.h>
#include <sys/sched.h>
#include <sys/service.h>
#include <sys/kernaddr.h>
struct req_ops tarfs_rops;
struct port_ops tarfs_pops;
void tarfs_port_init(void) {
	register_port(2, &tarfs_pops);

	uint64_t start = (uint64_t)&_binary_tarfs_start;
	uint64_t end = (uint64_t)&_binary_tarfs_end;
	uint64_t astart = ALIGN_UP(start, PAGE_SIZE_BIT);
	uint64_t aend = ALIGN(end, PAGE_SIZE_BIT);
	for (int i = astart; i < aend; i+=PAGE_SIZE)
		manage_phys_page(i-(uint64_t)&physoffset);
}
SERVICE_INIT(tarfs_port_init, tarfs_port);

int tarfs_port_connect(struct request *req, size_t len, void *buf) {
	req->data = NULL;
	req->rops = &tarfs_rops;
	return 0;
}
int tarfs_port_get_response(struct request *req, struct response *res) {
	uint64_t start = (uint64_t)&_binary_tarfs_start;
	uint64_t end = (uint64_t)&_binary_tarfs_end;
	uint64_t astart = ALIGN_UP(start, PAGE_SIZE_BIT);
	uint64_t aend = ALIGN(end, PAGE_SIZE_BIT);

	uint64_t ustart = ALIGN(start, PAGE_SIZE_BIT);
	uint64_t len = ALIGN_UP(end, PAGE_SIZE_BIT)-ustart;

	struct vm_area *vma = insert_vma_to_any(current->as, len, VMA_RW);
	uint64_t vaddr = vma->vma_begin;
	if (astart != start) {
		char *page = get_page();
		memset(page, 0, PAGE_SIZE);
		memcpy(page+(start-ustart), (void *)start, (astart-start));
		struct page *p = manage_page((uint64_t)page);
		address_space_assign_page(current->as, p, vaddr, PF_SHARED);
		page_unref(p, 0);
		vaddr += PAGE_SIZE;
	}
	for (int i = astart; i < aend; i+=PAGE_SIZE) {
		struct page *p = manage_phys_page(i-(uint64_t)&physoffset);
		address_space_assign_page(current->as, p, vaddr, PF_SNAPSHOT);
		page_unref(p, 0); //This page has a ref_count from tarfs_init
		vaddr += PAGE_SIZE;
	}
	if (aend != end) {
		char *page = get_page();
		memset(page, 0, PAGE_SIZE);
		memcpy(page, (void *)aend, end-aend);
		struct page *p = manage_page((uint64_t)page);
		address_space_assign_page(current->as, p, vaddr, PF_SHARED);
		page_unref(p, 0);
	}
	res->buf = (void *)(vma->vma_begin+(start-ustart));
	res->len = end-start;
	return 0;
}
struct port_ops tarfs_pops = {
	.connect = tarfs_port_connect,
};
struct req_ops tarfs_rops = {
	.get_response = tarfs_port_get_response,
};
