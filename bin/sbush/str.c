#include "str.h"
#include "string.h"
static inline struct str_block *
str_block_new(void) {
	struct str_block *sb = malloc(sizeof(struct str_block));
	sb->len = 0;
	sb->next = NULL;
	memset(sb->s, 0, sizeof(sb->s));
	return sb;
}
void append(struct str *s, char c) {
	s->len++;
	if (!s->head) {
		s->head = s->tail = str_block_new();
		s->head->len = 1;
		s->head->s[0] = c;
		return;
	}
	if (s->tail->len == 128) {
		s->tail->next = str_block_new();
		s->tail = s->tail->next;
	}
	s->tail->s[s->tail->len++] = c;
}
int dump(struct str *s, char **res) {
	*res = malloc(s->len+1);

	struct str_block *tmp;
	char *stmp = *res;
	for (tmp = s->head; tmp; tmp = tmp->next) {
		memcpy(stmp, tmp->s, tmp->len);
		stmp += tmp->len;
	}

	(*res)[s->len] = '\0';
	return s->len;
}

