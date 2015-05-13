#pragma once

#include <sys/defs.h>

struct elf_info;
struct elf_section_info;

//Load an elf, assuming addr to be the base of elf
struct elf_info *elf_load(char *addr);
//Get where this elf should be loaded
struct elf_section_info *elf_find_section(struct elf_info *, const char *name);
void *elf_get_section_load_base(struct elf_section_info *);
char *elf_get_section_addr(struct elf_section_info *);
uint64_t elf_get_section_size(struct elf_section_info *);
