#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_tsearch_##f
#else
#define  S(f)  _##f
#include <stdlib.h>
#include "musl-search.h"
#endif 

/* AVL tree height < 1.44*log2(nodes+2)-0.3, MAXH is a safe upper bound.  */
#define MAXH (sizeof(void*)*8*3/2)

struct S(node) {
  const void *key;
  void *a[2];
  int h;
};


static int height(void *n) { return n ? ((struct S(node) *)n)->h : 0; }

static int rot(void **p, struct S(node) *x, int dir /* deeper side */)
{
  struct S(node) *y = (struct S(node) *)x->a[dir];
  struct S(node) *z = (struct S(node) *)y->a[!dir];
  int hx = x->h;
  int hz = height(z);
  if (hz > height(y->a[dir])) {
    /*
     *   x
     *  / \ dir          z
     * A   y            / \
     *    / \   -->    x   y
     *   z   D        /|   |\
     *  / \          A B   C D
     * B   C
     */
    x->a[dir] = z->a[!dir];
    y->a[!dir] = z->a[dir];
    z->a[!dir] = x;
    z->a[dir] = y;
    x->h = hz;
    y->h = hz;
    z->h = hz+1;
  } else {
    /*
     *   x               y
     *  / \             / \
     * A   y    -->    x   D
     *    / \         / \
     *   z   D       A   z
     */
    x->a[dir] = z;
    y->a[!dir] = x;
    x->h = hz+1;
    y->h = hz+2;
    z = y;
  }
  *p = z;
  return z->h - hx;
}

/* balance *p, return 0 if height is unchanged.  */
static int __tsearch_balance(void **p)
{
  struct S(node) *n = (struct S(node) *)*p;
  int h0 = height(n->a[0]);
  int h1 = height(n->a[1]);
  if (h0 - h1 + 1u < 3u) {
    int old = n->h;
    n->h = h0<h1 ? h1+1 : h0+1;
    return n->h - old;
  }
  return rot(p, n, h0<h1);
}

void *musl_tsearch(void *cxt, const void *key, void **rootp,
  int (*cmp)(void *, const void *, const void *))
{
  if (!rootp)
    return 0;

  void **a[MAXH];
  struct S(node) *n = (struct S(node) *)*rootp;
  struct S(node) *r;
  int i=0;
  a[i++] = rootp;
  for (;;) {
    if (!n)
      break;
    int c = cmp(cxt, key, n->key);
    if (!c)
      return n;
    a[i++] = &n->a[c>0];
    n = (struct S(node) *)n->a[c>0];
  }
  r = (struct S(node) *)malloc(sizeof *r);
  if (!r)
    return 0;
  r->key = key;
  r->a[0] = r->a[1] = 0;
  r->h = 1;
  /* insert new node, rebalance ancestors.  */
  *a[--i] = r;
  while (i && __tsearch_balance(a[--i]));
  return r;
}

void musl_tdestroy(void * cxt, void *root, void (*freekey)(void *, void *))
{
  struct S(node) *r = (struct S(node) *)root;

  if (r == 0)
    return;
  musl_tdestroy(cxt, r->a[0], freekey);
  musl_tdestroy(cxt, r->a[1], freekey);
  if (freekey) freekey(cxt, (void *)r->key);
  free(r);
}

void *musl_tfind(void * cxt, const void *key, void *const *rootp,
  int(*cmp)(void * cxt, const void *, const void *))
{
  if (!rootp)
    return 0;

  struct S(node) *n = (struct S(node) *)*rootp;
  for (;;) {
    if (!n)
      break;
    int c = cmp(cxt, key, n->key);
    if (!c)
      break;
    n = (struct S(node) *)n->a[c>0];
  }
  return n;
}

static void walk(void * cxt, struct S(node) *r, 
                 void (*action)(void *, const void *, VISIT, int), int d)
{
  if (!r)
    return;
  if (r->h == 1)
    action(cxt, r, leaf, d);
  else {
    action(cxt, r, preorder, d);
    walk(cxt, (struct S(node) *)r->a[0], action, d+1);
    action(cxt, r, postorder, d);
    walk(cxt, (struct S(node) *)r->a[1], action, d+1);
    action(cxt, r, endorder, d);
  }
}

void musl_twalk(void * cxt, const void *root, 
                void (*action)(void *, const void *, VISIT, int))
{
  walk(cxt, (struct S(node) *)root, action, 0);
}


void *musl_tdelete(void * cxt, const void * key, void ** rootp,
  int(*cmp)(void * cxt, const void *, const void *))
{
  if (!rootp)
    return 0;

  void **a[MAXH+1];
  struct S(node) *n = (struct S(node) *)*rootp;
  struct S(node) *parent;
  struct S(node) *child;
  int i=0;
  /* *a[0] is an arbitrary non-null pointer that is returned when
     the root node is deleted.  */
  a[i++] = rootp;
  a[i++] = rootp;
  for (;;) {
    if (!n)
      return 0;
    int c = cmp(cxt, key, n->key);
    if (!c)
      break;
    a[i++] = &n->a[c>0];
    n = (struct S(node) *)n->a[c>0];
  }
  parent = (struct S(node) *)*a[i-2];
  if (n->a[0]) {
    /* free the preceding node instead of the deleted one.  */
    struct S(node) *deleted = n;
    a[i++] = &n->a[0];
    n = (struct S(node) *)n->a[0];
    while (n->a[1]) {
      a[i++] = &n->a[1];
      n = (struct S(node) *)n->a[1];
    }
    deleted->key = n->key;
    child = (struct S(node) *)n->a[0];
  } else {
    child = (struct S(node) *)n->a[1];
  }
  /* freed node has at most one child, move it up and rebalance.  */
  if (parent == n)
    parent = NULL;
  
  free(n);
  *a[--i] = child;
  while (--i && __tsearch_balance(a[i]));
  return parent;
}
