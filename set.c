#ifdef CEE_AMALGAMATION
#undef  S
#define S(f)    _cee_set_##f
#else
#define S(f)    _##f
#include "cee.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cee-internal.h"
#endif
#include "cee-header.h"
#include "musl-search.h"

struct S(header) {
  void * context;
  int (*cmp)(const void *l, const void *r);
  uintptr_t size;
  enum cee_del_policy del_policy;
  enum cee_trace_action ta;
  struct cee_sect cs;
  void * _[1];
};

#include "cee-resize.h"

static void S(free_pair_follow) (void * cxt, void * c) {
  enum cee_del_policy dp = * (enum cee_del_policy *) cxt;
  cee_del_e(dp, c);
}
    
static void S(trace_pair) (void * cxt, const void *nodep, const VISIT which, const int depth) {
  void * p;
  struct S(header) * h;
  switch (which) 
  {
    case preorder:
    case leaf:
      p = *(void **)nodep;
      cee_trace(p, *((enum cee_trace_action *)cxt));
      break;
    default:
      break;
  }
}

static void S(trace)(void * p, enum cee_trace_action ta) {
  struct S(header) * h = FIND_HEADER (p);
  switch (ta) {
    case trace_del_no_follow:
      musl_tdestroy(NULL, h->_[0], NULL);
      S(de_chain)(h);
      free(h);
      break;
    case trace_del_follow:
      musl_tdestroy(NULL, h->_[0], S(free_pair_follow));
      S(de_chain)(h);
      free(h);
      break;
    default:
      h->cs.gc_mark = ta - trace_mark;
      h->ta = ta;
      musl_twalk(&ta, h->_[0], S(trace_pair));
      break;
  }
}

int S(cmp) (void * cxt, const void * v1, const void *v2) {
  struct S(header) * h = (struct S(header) *) cxt;
  return h->cmp(v1, v2);
}

/*
 * create a new set and the equality of 
 * its two elements are decided by cmp
 * dt: specify how its elements should be handled if the set is deleted.
 */
struct cee_set * cee_set_mk_e (struct cee_state * st, enum cee_del_policy o, 
                  int (*cmp)(const void *, const void *)) 
{
  struct S(header) * m = (struct S(header) *)malloc(sizeof(struct S(header)));
  m->cmp = cmp;
  m->size = 0;
  ZERO_CEE_SECT(&m->cs);
  S(chain)(m, st);
  
  m->cs.trace = S(trace);
  m->cs.resize_method = resize_with_identity;
  m->cs.n_product = 1;
  m->context = NULL;
  m->_[0] = NULL;
  m->del_policy = o;
  return (struct cee_set *)m->_;
}

struct cee_set * cee_set_mk (struct cee_state * s, int (*cmp)(const void *, const void *)) {
  return cee_set_mk_e(s, CEE_DEFAULT_DEL_POLICY, cmp);
}

size_t cee_set_size (struct cee_set * s) {
  struct S(header) * h = FIND_HEADER(s);
  return h->size;
}

bool cee_set_empty (struct cee_set * s) {
  struct S(header) * h = FIND_HEADER(s);
  return h->size == 0;
}

/*
 * add an element key to the set m
 * 
 */
void cee_set_add(struct cee_set *m, void * val) {
  struct S(header) * h = FIND_HEADER(m);
  void ** oldp = (void **) musl_tsearch(h, val, h->_, S(cmp));
  
  if (oldp == NULL)
    cee_segfault();
  else if (*oldp != (void *)val)
    return;
  else {
    h->size ++;
    cee_incr_indegree(h->del_policy, val);
  }
  return;
}
    
static void S(del)(void * cxt, void * p) {
  enum cee_del_policy dp = *((enum cee_del_policy *)cxt);
  switch(dp) {
    case dp_del_rc:
      cee_del_ref(p);
      break;  
    case dp_del:
      cee_del(p);
      break;
    case dp_noop:
      break;
  }
}    
void cee_set_clear (struct cee_set * s) {
  struct S(header) * h = FIND_HEADER (s);
  musl_tdestroy(&h->del_policy, h->_[0], S(del));
  h->_[0] = NULL;
  h->size = 0;
}

void * cee_set_find(struct cee_set *m, void * key) {
  struct S(header) * h = FIND_HEADER(m);
  void **oldp = (void **) musl_tfind(h, key, h->_, S(cmp));
  if (oldp == NULL)
    return NULL;
  else
    return *oldp;
}

static void S(get_value) (void * cxt, const void *nodep, const VISIT which, const int depth) {
  void * p;
  switch (which) 
  {
    case preorder:
    case leaf:
      p = *(void **)nodep;
      cee_list_append((struct cee_list **)cxt, p);
      break;
    default:
      break;
  }
}

struct cee_list * cee_set_values(struct cee_set * m) {
  uintptr_t s = cee_set_size(m);
  struct S(header) * h = FIND_HEADER(m);
  h->context = cee_list_mk(h->cs.state, s);
  cee_use_realloc(h->context);
  musl_twalk(&h->context, h->_[0], S(get_value));
  return (struct cee_list *)h->context;
}

void * cee_set_remove(struct cee_set *m, void * key) {
  struct S(header) * h = FIND_HEADER(m);
  void ** old = (void **)musl_tfind(h, key, h->_, S(cmp));
  if (old == NULL)
    return NULL;
  else {
    h->size --;
    void * k = *old;
    musl_tdelete(h, key, h->_, S(cmp));
    return k;
  }
}

struct cee_set * cee_set_union_set (struct cee_state * s, struct cee_set * s1, struct cee_set * s2) {
  struct S(header) * h1 = FIND_HEADER(s1);
  struct S(header) * h2 = FIND_HEADER(s2);
  if (h1->cmp == h2->cmp) {
    struct cee_set * s0 = cee_set_mk(s, h1->cmp);
    struct cee_list * v1 = cee_set_values(s1);
    struct cee_list * v2 = cee_set_values(s2);
    int i;
    for (i = 0; i < cee_list_size(v1); i++)
      cee_set_add(s0, v1->_[i]);
    
    for (i = 0; i < cee_list_size(v2); i++)
      cee_set_add(s0, v2->_[i]);
    
    cee_del(v1);
    cee_del(v2);
    return s0;
  } else
    cee_segfault();
  return NULL;
}
