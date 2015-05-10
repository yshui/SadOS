#pragma once

struct service_init {
	void (*init)(void);
};

#define SERVICE_INIT(x, name) \
	static struct service_init \
	__attribute__((section("__services"), used)) \
	__name##_init = { \
		.init = x, \
	}
