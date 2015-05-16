/* No Copyright */
/* Public Domain */
#ifndef _STDLIB_H
#define _STDLIB_H

#include <sys/defs.h>
#include <vfs.h>

extern __thread int errno;
extern char **environ;

void exit(int status);

// memory
typedef uint64_t size_t;
void *malloc(size_t size);
void free(void *ptr);
int brk(void *end_data_segment);

// processes
typedef uint32_t pid_t;
pid_t fork(void);
pid_t getpid(void);
pid_t getppid(void);
int execve(const char *filename, char *const argv[], char *const envp[]);
int execvp(const char *filename, char *const argv[]);
pid_t waitpid(pid_t pid, int *status, int options);
unsigned int sleep(unsigned int seconds);
unsigned int alarm(unsigned int seconds);

// paths
char *getcwd(char *buf, size_t size);
int chdir(const char *path);

// files
typedef int64_t ssize_t;
enum { O_RDONLY = 0, O_WRONLY = 1, O_RDWR = 2, O_CREAT = 0x40, O_DIRECTORY = 0x10000 };
struct file* my_open(char *pathname, int flags);
ssize_t my_read(uint64_t fd, void *buf, size_t count);
void my_write(uint64_t fd, const void *buf, size_t count);
enum { SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2 };
typedef uint64_t off_t;
//lseek();
//off_t lseek(int fildes, off_t offset, int whence);
int my_close(struct file* fd);
int pipe(int filedes[2]);
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
struct dentry_reader* my_opendir(char *path);
struct dentry *my_readdir(struct dentry_reader* reader);
int my_closedir(struct dentry_reader* dir);

char *getenv(const char *);
int putenv(char *);

#endif
