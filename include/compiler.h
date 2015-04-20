#pragma once

#define __please_inline__ __attribute__((always_inline))

#define offsetof(type, member)  __builtin_offsetof (type, member)
#define container_of(ptr, type, member) ( __extension__ { \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type,member) );})

typedef _Bool bool;
