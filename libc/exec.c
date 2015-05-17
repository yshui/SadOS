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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <as.h>
#include <sendpage.h>
#include <thread.h>
#include <elf/elf.h>
#include <bitops.h>
#include "util.h"

extern char __cwd[];

static int mapper(const void *base, const struct elf64_phdr *ph, void *d) {
	if (ph->p_type == PT_DYNAMIC)
		return ENOEXEC;
	if (ph->p_type == PT_INTERP)
		return ENOEXEC;
	if (ph->p_type != PT_LOAD)
		return 0;

	uint64_t begin = ph->p_vaddr;
	uint64_t abegin = ALIGN(begin, 12);
	uint64_t aend = ALIGN_UP(begin+ph->p_memsz, 12);
	int as = *(int *)d;
	char *page = sendpage(0, 0, 0, aend-abegin);

	memcpy(page+(begin-abegin), base+ph->p_offset, ph->p_filesz);

	void *ret = sendpage(as, (uint64_t)page, abegin, aend-abegin);
	if ((long)ret < 0)
		return errno;
	return 0;
}
//Execute an in-memory elf image
int execvm(const char *base, char * const* argv, char * const *environ) {
	int as = asnew(0);
	//Parse elf and send pages to the new address space
	struct elf_info *ei = elf_load(base);
	if (!ei) {
		errno = ENOEXEC;
		return -1;
	}
	int ret = elf_foreach_ph(ei, &as, mapper);
	if (ret != 0) {
		asdestroy(as);
		errno = E2BIG;
		return -1;
	}

	//Allocate memory, put argument on it, then map it as stack
	uint64_t envsz = 0, envcnt = 0;
	for (int i = 0; environ[i]; i++) {
		envsz += strlen(environ[i])+1;
		envcnt++;
	}

	uint64_t argvsz = 0, argc = 0;
	for (int i = 0; argv[i]; i++) {
		argvsz += strlen(argv[i])+1;
		argc++;
	}

	uint64_t size = strlen(__cwd)+1+envsz+argvsz+2+8;
	char *page = sendpage(0, 0, 0, ALIGN_UP(size, 12));
	char *dataptr = page+4096-size+8;
	strcpy(dataptr, __cwd);
	dataptr += strlen(__cwd)+1;

	for (int i = 0; argv[i]; i++) {
		strcpy(dataptr, argv[i]);
		dataptr += strlen(argv[i])+1;
	}
	*(dataptr++) = 0;
	for (int i = 0; environ[i]; i++) {
		strcpy(dataptr, environ[i]);
		dataptr += strlen(environ[i])+1;
	}
	*(dataptr++) = 0;
	*(uint64_t *)(page+4096-size) = argc;

	page = sendpage(as, (uint64_t)page, 0, ALIGN_UP(size, 12));
	//Map another 1MB to stack
	sendpage(as, 0, (uint64_t)page-0x100000, 0x100000);

	//Set up thread info
	struct thread_info ti;
	memset(&ti, 0, sizeof(ti));
	ti.rsp = (uint64_t)(page+4096-size);
	ti.rcx = ei->hdr->e_entry;

	return create_task(as, &ti, CT_SELF);
}

int execvp(const char *name, char * const* argv) {
	char *env_path = strdup(getenv("PATH"));
	int npath, paths = 0;
	char **path = NULL;
	int flen = strlen(argv[0]);
	npath = split(env_path, ':', &path, &paths, SPLIT_NOESC);

	int i;
	int tmp_errno = 0;
	for (i = 0; i < npath; i++) {
		int plen = strlen(path[i]);
		char *buf = malloc(plen+flen+2);
		strcpy(buf, path[i]);
		buf[plen] = '/';
		buf[plen+1] = '\0';
		strcat(buf, argv[0]);
#ifdef __LIBC_DEBUG
		printf("Trying: %s\n", buf);
#endif

		execve(buf, argv, environ);
		if (errno != ENOENT || tmp_errno == 0)
			tmp_errno = errno;
	}
	errno = tmp_errno;
	return -1;
}
