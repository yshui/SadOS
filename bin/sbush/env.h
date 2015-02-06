#pragma once
#include "string.h"
static inline char *getenv(const char *name) {
	char **tmp = my_environ;
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

static inline int putenv(const char *p) {
	char *tp = strdup(p);
	char *eq = strchr(tp, '=');
	*eq = '\0';

	char *old = getenv(tp);
	old -= strlen(tp)+1;
	free(tp);

	char **tmp = my_environ;
	while(*tmp) {
		if (*tmp == old) {
			free(*tmp);
			*tmp = strdup(p);
			return 0;
		}
		tmp++;
	}

	if (nenv+2 > env_size) {
		char **new = malloc(env_size*2*sizeof(void *));
		if (!new)
			return -1;

		memset(new, 0, env_size*2*sizeof(void *));
		memcpy(new, my_environ, nenv*sizeof(void *));
		free(my_environ);
		my_environ = new;
		env_size = env_size*2;
	}

	my_environ[nenv] = strdup(p);
	if (!my_environ[nenv])
		return -1;

	nenv++;
	return 0;
}

static inline void env_init(char **envp) {
	int n = 0;
	char **tmp = envp;
	while(*tmp++)
		n++;
	my_environ = malloc(sizeof(void *)*(n+1));
	env_size = n+1;
	nenv = n;

	int i;
	for (i = 0; i < n; i++)
		my_environ[i] = strdup(envp[i]);
	my_environ[n] = NULL;
}
