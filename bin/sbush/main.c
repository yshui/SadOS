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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <uapi/ioreq.h>
#include <ipc.h>
#include <vfs.h>

int env_size, nenv;

#include "util.h"
#include "parse.h"
int pipe(int *p) {
	return -1;
}

const char *prompt = NULL;
static inline int builtin_export(struct pipe_part *p) {
	if (p->argc > 2) {
		printf("%s: invalid argument\n", p->argv0->str);
		return 1;
	}
	if (p->argc == 1) {
		char **tmp = environ;
		while(*tmp) {
			printf("%s\n", *tmp);
			tmp++;
		}
		return 0;
	}

	char *envp = p->argv0->next->str;
	int ret = putenv(strdup(envp));

	if (strncmp(envp, "PS1", 3) == 0)
		prompt = getenv("PS1");

	return ret;
}

static inline int builtin_cd(struct pipe_part *p) {
	if (p->argc != 2) {
		printf("%s: invalid arguments\n", p->argv0->str);
		return 1;
	}
	return chdir(p->argv0->next->str);
}

static inline int builtin_touch(struct pipe_part *p) {
	if (p->argc != 2) {
		printf("%s: invalid arguments\n", p->argv0->str);
		return 1;
	}
	return open(p->argv0->next->str, O_CREAT);
}


static inline int builtin_pwd(struct pipe_part *p) {
	char *pwd = getcwd(NULL, 0);
	if (pwd)
		printf("%s\n", pwd);
	else
		return 1;
	free(pwd);
	return 0;
}

static inline int builtin_echo(struct pipe_part *p) {
	struct argv_part *n = p->argv0;
	while(n) {
		printf("%s ", n->str);
		n = n->next;
	}
	printf("\n");
	return 0;
}

static inline int builtin_exit(struct pipe_part *p) {
	exit(0);
	return 0;
}
static inline int builtin_ls(struct pipe_part *p)
{
    //printf("In ls here: %s\n", pathname);
    //char *pathname = p -> argv0 -> next -> str;
    char *pathname = getcwd(NULL, 0);
    printf("Opendir %s\n", pathname);
    void *fd = opendir(pathname);
    //struct dentry_reader *p = NULL;
    struct dirent *de;
    if (fd < 0)
    {
        printf("Cannot open directory %s.\n", pathname);
        return -1;
    }
    do {
        de = readdir(fd);
        //printf("ret : %d\n", ret);
        if (de)
            printf("%s\n", de->d_name);
    }
    while(de);
    return 0;
}

static inline int builtin_cat(struct pipe_part *p)
{
    char *pathname = p -> argv0 -> next -> str;
    //printf("Real path name for cat: %s\n", cur_dir);
    int fd = open(pathname, 0);
    if (fd < 0)
    {
        printf("Error: file doesn't exist\n");
        return -1;
    }
    uint64_t fd_int = (uint64_t) fd;
    //printf("fd: %d\n", fd);
    char buf[513];
    int count;

    while ( (count = read(fd_int, buf, 513) ) != 0)
        printf("%s", buf);
    printf("\n");
    return 0;
}

static inline int builtin_help(struct pipe_part *p) {
	printf("touch file: touch\n");
	printf("echo args ...: echo arguments\n");
	printf("export name=value: Change environment variable name to value\n");
	printf("export: List all environment variables\n");
	printf("cd path: Change working directory\n");
	printf("pwd: Get current working directory\n");
	printf("path/to/binary: Execute binary file\n");
	printf("binary: Execute binary in PATH\n");
    printf("cat: show file content\n");
    printf("ls: list files in a path\n");
	printf("path/to/binary1 | path/to/binary2 | ...: Execute pipelines of "
	    "binaries\n");
	return 0;
}

static inline int exec_with_fd(int infd, int outfd, char **argv) {
	int ret = fork();
	if (ret < 0) {
		printf("%s: failed to fork\n", argv[0]);
		return 1;
	}
	if (ret) {
		close(infd);
		close(outfd);
		return 0;
	}

	ret = dup2(infd, STDIN_FILENO);
	if (ret < 0) {
		printf("%s: failed to dup file descripter\n", argv[0]);
		exit(1);
	}
	close(infd);

	ret = dup2(outfd, STDOUT_FILENO);
	if (ret < 0) {
		printf("%s: failed to dup file descripter: %s\n", argv[0]);
		exit(1);
	}
	close(outfd);

	char *slash = strchr(argv[0], '/');
	if (slash) {
		ret = execve(argv[0], argv, environ);
	} else
		ret = execvp(argv[0], argv);
	if (ret < 0) {
		if (errno == EACCES || errno == EPERM)
			printf("%s: permission denied\n", argv[0]);
		else if (errno == ENOENT)
			printf("%s: command not found\n", argv[0]);
		else
			printf("%s: failed to execv\n", argv[0]);
	}
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	exit(1);
	return 0;
}

static inline int pipe_exec(struct cmd *c) {

	//Create a number of pipes
	int i;
	int (*p)[2] = malloc(sizeof(int)*2*(c->npipe+1));
	for (i = 1; i < c->npipe; i++)
		pipe(p[i]);
	p[0][0] = dup(STDIN_FILENO);
	p[c->npipe][1] = dup(STDOUT_FILENO);

	int child = 0;
	struct pipe_part *pp;
	i = 0;
	for (pp = c->pipe0; pp; pp = pp->next, i++) {
		int ret, an;
		struct argv_part *ap;
		char **argv = malloc(sizeof(void *)*(pp->argc+1));
		for(an = 0, ap = pp->argv0; ap; ap = ap->next)
			argv[an++] = ap->str;
		argv[an] = NULL;
		ret = exec_with_fd(p[i][0], p[i+1][1], argv);
		if (ret == 0)
			child++;
		free(argv);
	}
	free(p);

	while(child--) {
		//int status;
		//int __attribute__((unused)) pid = waitpid(-1, &status, 0);
#ifdef __DEBUG
		printf("PID=%d, status=%d\n", pid, status);
#endif
	}
	return 0;
}

struct builtin_cmd {
	const char *name;
	int (*handler)(struct pipe_part *p);
} cmd_hanlder[] = {
	{ .name = "touch", .handler = builtin_touch },
	{ .name = "export", .handler = builtin_export },
	{ .name = "cd", .handler = builtin_cd },
	{ .name = "pwd", .handler = builtin_pwd },
	{ .name = "exit", .handler = builtin_exit },
	{ .name = "quit", .handler = builtin_exit },
	{ .name = "help", .handler = builtin_help },
	{ .name = "echo", .handler = builtin_echo },
    { .name = "ls", .handler = builtin_ls},
    { .name = "cat", .handler = builtin_cat},
	{ .name = NULL, .handler = NULL }
};
static int batch_mode = false;
int main(int argc, char **argv, char **envp) {
    //file writing
//    int fd = open("/tarfs/test/f", 0);
//    int fd1 = open("/sata/f", O_CREAT);
    //printf("fd assigned: %d\n", fd);
//    char buf[513];
    //copying a file
//    while (read(fd, buf, 512) != 0)
 //       write(fd1, buf, 512);

	if (argc > 1) {
		batch_mode = true;
		int infd = open(argv[1], O_RDONLY);
		if (infd < 0) {
			printf("Failed to open %s\n", argv[1]);
			return 1;
		}
		int ret = dup2(infd, 0);
		if (ret < 0) {
			printf("Failed to dup2\n");
			return 1;
		}
	} else {
		//Send an io_req to become the foreground process
		struct io_req req;
		req.type = 3;
		int cookie = request(0, sizeof(req), &req);
		printf("FF %d\n", cookie);
		close(cookie);
	}
	if (!batch_mode)
		printf("Type 'help' to list available commands\n");
	prompt = getenv("PS1");
	if (!prompt)
		prompt = "trash%";

	do {
		if (!batch_mode)
			printf("%s", prompt);

		struct cmd *c = parse();
		if (!c) {
			printf("EOF, quit\n");
			break;
		}
		if (c->npipe == 0)
			continue;

		int i = 0;
		char *argv0 = c->pipe0->argv0->str;
		while(cmd_hanlder[i].name) {
			if (strcmp(cmd_hanlder[i].name, argv0) == 0) {
				if (c->npipe > 1)
					printf("piping builtin commands not supported\n");
				cmd_hanlder[i].handler(c->pipe0);
				break;
			}
			i++;
		}

		if (cmd_hanlder[i].name == NULL)
			//not a builtin command
			pipe_exec(c);
		cmd_free(c);
	}while(true);
}
