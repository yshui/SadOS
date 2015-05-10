#pragma once

#define __please_inline__ __attribute__((always_inline))

#define offsetof(type, member)  __builtin_offsetof (type, member)
#define container_of(ptr, type, member) ( __extension__ { \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type,member) );})

typedef _Bool bool;

#define true (1)
#define false (0)

#define atomic_add(x, v) __sync_fetch_and_add(&(x), (v))
#define atomic_add_post(x, v) __sync_add_and_fetch(&(x), (v))
#define atomic_sub(x, v) __sync_fetch_and_sub(&(x), (v))
#define atomic_sub_post(x, v) __sync_sub_and_fetch(&(x), (v))
#define atomic_inc(x) atomic_add(x, 1)
#define atomic_dec(x) atomic_sub(x, 1)
#define atomic_dec_post(x) atomic_sub_post(x, 1)
