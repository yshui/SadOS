#ifndef _STDLIB_H
#define _STDLIB_H

extern int errno;

int main(int argc, char* argv[], char* envp[]);
void exit(int status);

// memory
typedef unsigned long size_t;
void *malloc(size_t size);
void free(void *ptr);
int brk(void *end_data_segment);

// processes
typedef unsigned pid_t;
pid_t fork(void);
pid_t getpid(void);
pid_t getppid(void);
int execve(const char *filename, char *const argv[], char *const envp[]);
int pipe(int filedes[2]);
pid_t waitpid(pid_t pid, int *status, int options);
unsigned int sleep(unsigned int seconds);
unsigned int alarm(unsigned int seconds);

// paths
char *getcwd(char *buf, size_t size);
int chdir(const char *path);

// files
typedef long ssize_t;
enum { O_RDONLY = 0, O_WRONLY = 1, O_RDWR = 2, O_CREAT = 64 };
int open(const char *pathname, int flags);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
enum { SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2 };
typedef int off_t;
off_t lseek(int fildes, off_t offset, int whence);
int close(int fd);
int dup(int oldfd);
int dup2(int oldfd, int newfd);

// directories
#define NAME_MAX 255
struct dirent
{
	long d_ino;
	off_t d_off;
	unsigned short d_reclen;
	char d_name [NAME_MAX+1];
};
void *opendir(const char *name);
int readdir(unsigned int fd, struct dirent *dirp, unsigned int count);
int closedir(void *dir);

// sockets
int socket(int domain, int type, int protocol);
struct sockaddr {
	unsigned short sa_family;
	char           sa_data[14];
};
typedef unsigned int socklen_t;
int bind(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen);
int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

#endif
