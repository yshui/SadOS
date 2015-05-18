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
#include <elf/elf.h>
#include <stdlib.h>
#include <string.h>
#include "header.h"

#define EI_CLASS    (4)
#define EI_DATA     (5)

#define ELFCLASS64  (2)
#define ELFDATA2LSB (1)

const char elf_magic[] = {
	'\x7f', 'E', 'L', 'F',
};

struct elf_info *elf_load(const char *addr) {
	struct elf_info *ei = malloc(sizeof(struct elf_info));
	ei->hdr = ei->base = addr;
	if (strncmp((char *)ei->hdr->e_ident, elf_magic, 4) != 0)
		goto err_out;

	if (ei->hdr->e_ident[EI_CLASS] != ELFCLASS64)
		goto err_out;
	if (ei->hdr->e_ident[EI_DATA] != ELFDATA2LSB)
		goto err_out;
	return ei;
err_out:
	free(ei);
	return NULL;
}

struct elf_section_info *elf_find_section(struct elf_info *ei, const char *name) {
	struct elf_section_info *esi = malloc(sizeof(struct elf_section_info));

	const struct elf64_shdr *hdr_table = ei->base+ei->hdr->e_shoff;
	const char *strt = ei->base+hdr_table[ei->hdr->e_shstrndx].sh_offset;

	int i;
	for (i = 0; i < ei->hdr->e_shnum; i++)
		if (strcmp(name, strt+hdr_table[i].sh_name) == 0)
			break;

	if (i >= ei->hdr->e_shnum) {
		free(esi);
		return NULL;
	}
	esi->section_base = ei->base+hdr_table[i].sh_offset;
	esi->hdr = hdr_table+i;
	return esi;
}

int elf_foreach_ph(struct elf_info *ei, void *d, ph_fn fn) {
	const struct elf64_phdr *phdr_table = ei->base+ei->hdr->e_phoff;
	for (int i = 0; i < ei->hdr->e_phnum; i++) {
		int ret = fn(ei->base, phdr_table+i, d);
		if (ret)
			return ret;
	}
	return 0;
}
