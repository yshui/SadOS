#include <stdio.h>
#include <sys/printk.h>
#include <stdlib.h>
#include <string.h>
//#include <vfs.h>

char cwd[200] = "/";
char *new_getcwd(char *buf, size_t size)
{
    strcpy(buf, cwd);
    return buf;
}
int new_chdir(const char *path)
{
    //absolute path
    if (path[0] == '/')
        strcpy(cwd, path);
    else
    {
        new_getcwd(cwd, 1000);
        strcat(cwd, "/");
        strcat(cwd, path);
    }
    printk("change dir: %s\n", cwd);
    return 0;
}

