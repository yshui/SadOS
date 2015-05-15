#pragma once
void aspace_init(struct address_space *ret, void *pml4, int flags);
void unassign_page(struct page_entry *pe);
void remove_vma_range(struct address_space *as, uint64_t base, uint64_t len);
