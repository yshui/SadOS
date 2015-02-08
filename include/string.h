#pragma once

#include <stdlib.h>

char *strchr(const char *, char);
size_t strlen(const char *);
char *strdup(const char *);
int strncmp(const char *, const char *, int);
int strcmp(const char *, const char *);
char *strcpy(char *, const char *);
char *strcat(char *, const char *);

void *memcpy(void *, const void *, size_t);
void *memset(void *, char, size_t);
void *memmove(void *, const void *, size_t);
void bcopy(const void *, void *, size_t);
