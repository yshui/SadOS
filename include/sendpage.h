#pragma once
void *sendpage(int as, uint64_t src, uint64_t dst, size_t len);
int munmap(void *base, size_t len);
