/* No Copyright */
/* Public Domain */
#ifndef _STDIO_H
#define _STDIO_H

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int printf(const char *format, ...);
int scanf(const char *format, ...);
char getchar(void);

#endif
