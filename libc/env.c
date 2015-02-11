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
#include <string.h>

char **environ = NULL;

static size_t env_size = 0, nenv = -1;
static int __env_init = 0;

static inline void env_init(void) {
	int n = 0;
	char **tmp = environ;
	while(*tmp++)
		n++;
	tmp = environ;
	environ = malloc(sizeof(void *)*(n+1));
	env_size = n+1;
	nenv = n;
	__env_init = 1;

	int i;
	for (i = 0; i < n; i++)
		environ[i] = strdup(tmp[i]);
	environ[n] = NULL;
}

char *getenv(const char *name) {
	if (!__env_init)
		env_init();
	char **tmp = environ;
	int len = strlen(name);
	while(*tmp) {
		if (strncmp(*tmp, name, len) == 0) {
			if ((*tmp)[len] == '=')
				return (*tmp)+len+1;
		}
		tmp++;
	}
	return NULL;
}

int putenv(char *p) {
	if (!__env_init)
		env_init();
	char *tp = strdup(p);
	char *eq = strchr(tp, '=');
	*eq = '\0';

	char *old = getenv(tp);
	old -= strlen(tp)+1;
	free(tp);

	char **tmp = environ;
	while(*tmp) {
		if (*tmp == old) {
			free(*tmp);
			*tmp = p;
			return 0;
		}
		tmp++;
	}

	if (nenv+2 > env_size) {
		char **new = malloc(env_size*2*sizeof(void *));
		if (!new)
			return -1;

		memset(new, 0, env_size*2*sizeof(void *));
		memcpy(new, environ, nenv*sizeof(void *));
		free(environ);
		environ = new;
		env_size = env_size*2;
	}

	environ[nenv] = p;
	if (!environ[nenv])
		return -1;

	nenv++;
	return 0;
}

