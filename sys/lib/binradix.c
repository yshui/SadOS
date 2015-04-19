//Fix-length, binary radix tree
#include <sys/defs.h>
#include <sys/mm.h>
struct binradix_node {
	struct binradix_node *next[2];
	void *data;
	uint64_t part;
	uint8_t len;
};

static inline void binradix_root_init(struct binradix_node *root) {
	root->next[0] = root->next[1] = NULL;
	root->data = obj_pool_create(sizeof(struct binradix_node));
	root->len = 0;
}

static inline int hibit(uint64_t x) {
	return __builtin_clzll(x);
}

static inline uint64_t bincmp(uint64_t a, uint64_t b, uint8_t off, uint8_t nbit) {
	uint64_t x = a^b;
	x &= (2ull<<off)-1;
	if (off-nbit < 63)
		x &= ~((1ull<<(off-nbit+1))-1);
	else
		return 0;
	return x;
}

static inline int bineq(uint64_t a, uint64_t b, uint8_t off, uint8_t nbit) {
	if (!nbit)
		return 1;
	return bincmp(a, b, off, nbit) == 0;
}

void *binradix_find(struct binradix_node *root, uint64_t key) {
	struct binradix_node *now = root;
	uint8_t key_off = 63;
	while (now) {
		if (bineq(now->part, key, key_off, now->len)) {
			if (key_off >= now->len) {
				key_off -= now->len;
				now = now->next[(key>>key_off)&1];
			} else
				return now->data;
		} else
			return NULL;
	}
	return NULL;
}

void *binradix_insert(struct binradix_node *root,
		      uint64_t key, void *data) {
	struct binradix_node *now = root;
	struct obj_pool *obp = root->data;
	uint64_t key_off_s = 1ull<<63;
	int key_off = 63;
	while(now) {
		uint64_t x = bincmp(now->part, key, key_off, now->len);
		int hb = 63-hibit(x), bit;
		struct binradix_node *new;

		if (x) {
			int len = key_off-hb;
			key_off_s >>= len;
			key_off = hb;

			bit = !!(key&key_off_s);
			//Split nodes
			new = obj_pool_alloc(obp);
			new->part = now->part;
			new->len = now->len-len;
			new->next[0] = now->next[0];
			new->next[1] = now->next[1];
			new->data = now->data;
			now->next[!bit] = new;
			now->len = len;
			now->data = NULL;

			new = obj_pool_alloc(obp);
		} else {
			key_off_s >>= now->len;
			key_off -= now->len;

			//Full match
			bit = !!(key&key_off_s);
			new = now->next[bit];
			if (!key_off_s)
				//Duplicate key
				return now->data;
			if (!new)
				new = obj_pool_alloc(obp);
			else {
				now = new;
				continue;
			}
		}

		now->next[bit] = new;
		new->part = key;
		new->len = key_off+1;
		new->next[0] = new->next[1] = NULL;
		new->data = data;
		break;
	}
	return NULL;
}

void *binradix_delete(struct binradix_node *root, uint64_t key) {
	struct binradix_node *now = root, *parent = NULL;
	int key_off = 63;
	while(now) {
		int bit;
		if (bineq(now->part, key, key_off, now->len)) {
			if (key_off < now->len)
				break;
			key_off -= now->len;
			bit = (key>>key_off)&1;
			parent = now;
			now = now->next[bit];
		} else
			return NULL;
	}
	int bit = (now == parent->next[1]);
	void *ret = now->data;
	parent->next[bit] = NULL;
	obj_pool_free(root->data, now);
	if (parent != root) {
		struct binradix_node *to_merge = parent->next[!bit];
		uint64_t mask = (2ull<<key_off)-1;
		parent->next[0] = to_merge->next[0];
		parent->next[1] = to_merge->next[1];
		parent->part &= ~mask;
		parent->part |= (to_merge->part)&mask;
		parent->len += to_merge->len;
		parent->data = to_merge->data;
		obj_pool_free(root->data, to_merge);
	}
	return ret;
}
struct binradix_node *binradix_new(void) {
	struct obj_pool *obp = obj_pool_create(sizeof(struct binradix_node));
	struct binradix_node *ret = obj_pool_alloc(obp);
	ret->next[0] = ret->next[1] = NULL;
	ret->data = obp;
	ret->len = 0;
	return ret;
}
