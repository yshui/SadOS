#pragma once
#include <sys/defs.h>
struct binradix_node;
void *binradix_find(struct binradix_node *, uint64_t);
void *binradix_insert(struct binradix_node *, uint64_t, void *);
struct binradix_node *binradix_new(void);
void *binradix_delete(struct binradix_node *root, uint64_t key);

typedef void (*visitor_fn)(void *node, void *d);
void binradix_for_each(struct binradix_node *, void *, visitor_fn);
