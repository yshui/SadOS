/*
   An implementation of top-down splaying
   D. Sleator <sleator@cs.cmu.edu>
   March 1992

   "Splay trees", or "self-adjusting search trees" are a simple and
   efficient data structure for storing an ordered set.  The data
   structure consists of a binary tree, without parent pointers, and no
   additional fields.  It allows searching, insertion, deletion,
   deletemin, deletemax, splitting, joining, and many other operations,
   all with amortized logarithmic performance.  Since the trees adapt to
   the sequence of requests, their performance on real access patterns is
   typically even better.  Splay trees are described in a number of texts
   and papers [1,2,3,4,5].

   The code here is adapted from simple top-down splay, at the bottom of
   page 669 of [3].  It can be obtained via anonymous ftp from
   spade.pc.cs.cmu.edu in directory /usr/sleator/public.

   The chief modification here is that the splay operation works even if the
   item being splayed is not in the tree, and even if the tree root of the
   tree is NULL.  So the line:

   t = splay(i, t);

   causes it to search for item with key i in the tree rooted at t.  If it's
   there, it is splayed to the root.  If it isn't there, then the node put
   at the root is the last one before NULL that would have been reached in a
   normal binary search for i.  (It's a neighbor of i in the tree.)  This
   allows many other operations to be easily implemented, as shown below.

   [1] "Fundamentals of data structures in C", Horowitz, Sahni,
   and Anderson-Freed, Computer Science Press, pp 542-547.
   [2] "Data Structures and Their Algorithms", Lewis and Denenberg,
   Harper Collins, 1991, pp 243-251.
   [3] "Self-adjusting Binary Search struct splay_nodes" Sleator and Tarjan,
   JACM Volume 32, No 3, July 1985, pp 652-686.
   [4] "Data Structure and Algorithm Analysis", Mark Weiss,
   Benjamin Cummins, 1992, pp 119-130.
   [5] "Data Structures, Algorithms, and Performance", Derick Wood,
   Addison-Wesley, 1993, pp 367-375.
   */

/* Adapted by Yuxuan Shui */
/* No license/copyright in original source file, assuming public domain */

#include <sys/defs.h>
#include <sys/mm.h>
#include <sys/lib/splay.h>

struct splay {
	struct splay_node *root;
	struct obj_pool *obp;
	splay_cmp cmp;
};
struct splay_node {
	struct splay_node *left, *right;
	void *data;
};
static void splay (void *data, struct splay_node **rptr, splay_cmp cmp) {
	/* Simple top down splay, not requiring i to be in the tree t.  */
	/* What it does is described above.                             */
	struct splay_node tmp_root, *l, *r, *tmp, *t = *rptr;
	if (t == NULL) return;
	tmp_root.left = tmp_root.right = NULL;
	l = r = &tmp_root;

	for (;;) {
		if (cmp(data, t->data) < 0) {
			if (t->left == NULL) break;
			if (cmp(data, t->left->data) < 0) {
				/* rotate right */
				tmp = t->left;
				t->left = tmp->right;
				tmp->right = t;
				t = tmp;
				if (t->left == NULL) break;
			}
			/* link right */
			r->left = t;
			r = t;
			t = t->left;
		} else if (cmp(data, t->data) > 0) {
			if (t->right == NULL) break;
			if (cmp(data, t->right->data) > 0) {
				/* rotate left */
				tmp = t->right;
				t->right = tmp->left;
				tmp->left = t;
				t = tmp;
				if (t->right == NULL) break;
			}
			/* link left */
			l->right = t;
			l = t;
			t = t->right;
		} else
			break;
	}
	l->right = t->left;
	r->left = t->right;
	t->left = tmp_root.right;
	t->right = tmp_root.left;
	*rptr = t;
}

void *splay_insert(struct splay *s, void *data) {
	/* Insert i into the tree t, unless it's already there.    */
	/* Return a pointer to the resulting tree.                 */
	struct splay_node *new;

	if (s->root == NULL) {
		new = (struct splay_node *)obj_pool_alloc(s->obp);
		new->data = data;
		new->left = new->right = NULL;
		s->root = new;
		return NULL;
	}

	splay(data, &s->root, s->cmp);

	struct splay_node *t = s->root;
	if (s->cmp(data, t->data) == 0)
		return t->data;

	new = (struct splay_node *)obj_pool_alloc(s->obp);
	new->data = data;
	if (s->cmp(data, t->data) < 0){
		new->left = t->left;
		new->right = t;
		t->left = NULL;
		s->root = new;
	} else if (s->cmp(data, t->data) > 0){
		new->right = t->right;
		new->left = t;
		t->right = NULL;
		s->root = new;
	}
	return NULL;
}

void *splay_delete(struct splay *s, void *data) {
	/* Deletes i from the tree if it's there.               */
	/* Return a pointer to the resulting tree.              */
	if (s->root == NULL) return NULL;
	splay(data, &s->root, s->cmp);

	struct splay_node *t = s->root;
	if (s->cmp(data, t->data) == 0) {
		if (t->left == NULL) {
			s->root = t->right;
		} else {
			splay(data, &t->left, s->cmp);
			t->left->right = t->right;
			s->root = t->left;
		}

		void *ret = t->data;
		obj_pool_free(s->obp, t);
		return ret;
	}
	return NULL;
}

void *splay_prev(struct splay *s, void *data) {
	//Find the last element that is less than data
	if (s->root == NULL) return NULL;

	splay(data, &s->root, s->cmp);

	struct splay_node *t = s->root;
	if (s->cmp(data, t->data) >= 0) {
		if (t->left == NULL)
			return NULL;
		splay(data, &t->left, s->cmp);
		return t->left->data;
	} else
		return t->data;
}

void *splay_next(struct splay *s, void *data) {
	//Find the first element that is greater than data
	if (s->root == NULL) return NULL;

	splay(data, &s->root, s->cmp);

	struct splay_node *t = s->root;
	if (s->cmp(data, t->data) <= 0) {
		if (t->right == NULL)
			return NULL;
		splay(data, &t->right, s->cmp);
		return t->right->data;
	} else
		return t->data;

}

void *splay_find(struct splay *s, void *data) {
	//Find the first element that is greater than data
	if (s->root == NULL) return NULL;

	splay(data, &s->root, s->cmp);

	struct splay_node *t = s->root;
	if (s->cmp(data, t->data) == 0)
		return t->data;
	return NULL;
}

struct splay *splay_new(splay_cmp cmp) {
	struct obj_pool *obp = obj_pool_new(sizeof(struct splay_node));
	struct splay *ret = obj_pool_alloc(obp); //struct splay is smaller or equal to struct splay_node
	ret->cmp = cmp;
	ret->obp = obp;
	ret->root = NULL;
	return ret;
}
