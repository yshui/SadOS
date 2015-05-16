#pragma once

struct splay;
struct splay_node;

typedef int (*splay_cmp)(const void *, const void *);

void *splay_next(struct splay *s, void *data);
void *splay_prev(struct splay *s, void *data);
void *splay_delete(struct splay *s, void *data);
void *splay_insert(struct splay *s, void *data);
void *splay_find(struct splay *s, void *data);
struct splay *splay_new(splay_cmp);
