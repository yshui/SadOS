#include <sys/idt.h>
#include <sys/printk.h>
#include <sys/cpu.h>
#include <sys/panic.h>
#include <sys/mm.h>
#include <sys/sched.h>
#include <sys/interrupt.h>
#include <bitops.h>
#include <string.h>
extern void (*page_fault_entry)(void);
struct page *zero_page;
uint64_t page_fault_handler(uint64_t err) {
	uint64_t vaddr = read_cr2();
	printk("%d: %p\n", err, vaddr);
	if (!GET_BIT(err, 2))
		panic("Page fault in the kernel");
	//Do we have a page allocated for it?
	uint64_t page = ALIGN(vaddr, PAGE_SIZE_BIT); //Align to page boundary
	disable_interrupts();
	struct vm_area *vma = vma_find_by_vaddr(current->as, vaddr);
	if (PTR_IS_ERR(vma)) {
		printk("User space trying to access invalid memory, pid=%d\n", current->pid);
		kill_current(1);
		return 1; //Reschedule
	}

	struct page_entry *pe = get_allocated_page(current->as, page);
	if (!pe) {
		address_space_assign_page(current->as, zero_page, page, PF_SNAPSHOT);
		pe = get_allocated_page(current->as, page);
		if (!pe)
			panic("Can't get back the page we just inserted\n");
	}

	if (err & PTE_W) {
		if (!vma->vma_flags & VMA_RW) {
			printk("Writing to read-only page\n");
			kill_current(1);
			return 1;
		}
		if (pe->p->snap_count > 0)
			//Has snapshot, unshare this
			unshare_page(pe);
	}
	address_space_map(page);
	enable_interrupts();
	printk("Return to user space\n");

	return 0;

}
void page_fault_init(void) {
	//Register page_fault_handle, 0xe=page fault, 0xf=trap gate
	register_handler(0xe, &page_fault_entry, 0xf, 1);

	zero_page = manage_phys_page(get_phys_page());
	memset((void *)(zero_page->phys_addr+KERN_VMBASE), 0, PAGE_SIZE);
}
