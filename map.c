#ifdef CEE_AMALGAMATION
#undef  S
#define S(f)  _cee_map_##f
#else
#define S(f)  _##f
#include "cee.h"
#include "cee-internal.h"
#include <stdlib.h>
#include <string.h>
#endif
#include "cee-header.h"
#include "musl-search.h"


struct S(header) {
  void * context;
  int (*cmp)(const void *l, const void *r);
  uintptr_t size;
  enum cee_del_policy key_del_policy;
  enum cee_del_policy val_del_policy;
  enum cee_trace_action ta;
  struct cee_sect cs;
  void * _[1];
};
    
#include "cee-resize.h"

static void S(free_pair_follow)(void * cxt, void * c) {
  cee_del(c);
}
        
static void S(trace_pair) (void * cxt, const void *nodep, const VISIT which, const int depth) {
  struct cee_tuple * p;
  struct S(header) * h;
  switch (which) 
  {
    case preorder:
    case leaf:
      p = (struct cee_tuple *)*(void **)nodep;
      cee_trace(p, *(enum cee_trace_action *)cxt);
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
      musl_tdestroy((void *)&ta, h->_[0], S(free_pair_follow));
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

static int S(cmp) (void * cxt, const void * v1, const void * v2) {
  struct S(header) * h = (struct S(header) *) cxt;
  struct cee_tuple * t1 = (struct cee_tuple *) v1;
  struct cee_tuple * t2 = (struct cee_tuple *) v2;
  return h->cmp(t1->_[0], t2->_[0]);
}

struct cee_map * cee_map_mk_e (struct cee_state * st, enum cee_del_policy o[2], 
                  int (*cmp)(const void *, const void *)) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * m = malloc(mem_block_size);
  m->context = NULL;
  m->cmp = cmp;
  m->size = 0;
  ZERO_CEE_SECT(&m->cs);
  S(chain)(m, st);
  
  m->cs.trace = S(trace);
  m->cs.resize_method = resize_with_identity;
  m->cs.mem_block_size = mem_block_size;
  m->cs.cmp = 0;
  m->cs.cmp_stop_at_null = 0;
  m->cs.n_product = 2; // key, value
  
  m->key_del_policy = o[0];
  m->val_del_policy = o[1];
  m->_[0] = 0;
  return (struct cee_map *)m->_;
}

struct cee_map * cee_map_mk(struct cee_state * st, int (*cmp) (const void *, const void *)) {
  static enum cee_del_policy d[2] = { CEE_DEFAULT_DEL_POLICY, CEE_DEFAULT_DEL_POLICY };
  return cee_map_mk_e(st, d, cmp);
}
    
uintptr_t cee_map_size(struct cee_map * m) {
  struct S(header) * b = FIND_HEADER(m);
  return b->size;
}

void cee_map_add(struct cee_map * m, void * key, void * value) {
  struct S(header) * b = FIND_HEADER(m);
  
  enum cee_del_policy d[2];
  d[0] = b->key_del_policy;
  d[1] = b->val_del_policy;
  
  struct cee_tuple * t = cee_tuple_mk_e(b->cs.state, d, key, value);
  struct cee_tuple ** oldp = (struct cee_tuple **)musl_tsearch(b, t, b->_, S(cmp));
  if (oldp == NULL)
    cee_segfault(); // run out of memory
  else if (*oldp != t) 
    cee_del(t);
  else
    b->size ++;
  return;
}

void * cee_map_find(struct cee_map * m, void * key) {
  struct S(header) * b = FIND_HEADER(m);
  struct cee_tuple t = { key, 0 };
  struct cee_tuple **pp = (struct cee_tuple **)musl_tfind(b, &t, b->_, S(cmp));
  if (pp == NULL)
    return NULL;
  else {
    struct cee_tuple * p = *pp;
    return p->_[1];
  }
}

void * cee_map_remove(struct cee_map * m, void * key) {
  struct S(header) * b = FIND_HEADER(m);
  void ** oldp = (void **)musl_tdelete(b, key, b->_, S(cmp));
  if (oldp == NULL)
    return NULL;
  else {
    b->size --;
    struct cee_tuple * ret = (struct cee_tuple *)*oldp;
    cee_del(ret);
    cee_decr_indegree(b->key_del_policy, ret->_[0]);
    cee_decr_indegree(b->val_del_policy, ret->_[1]);
    return ret->_[1];
  }
}

static void S(get_key) (void * cxt, const void *nodep, const VISIT which, const int depth) {
  struct cee_tuple * p;
  switch (which) 
  {
    case preorder:
    case leaf:
      p = *(struct cee_tuple **)nodep;
      cee_list_append((struct cee_list **)cxt, p->_[0]);
      break;
    default:
      break;
  }
}

struct cee_list * cee_map_keys(struct cee_map * m) {
  uintptr_t n = cee_map_size(m);
  struct S(header) * b = FIND_HEADER(m);
  struct cee_list * keys = cee_list_mk(b->cs.state, n);
  b->context = keys;
  musl_twalk(&keys, b->_[0], S(get_key));
  return keys;
}


static void S(get_value) (void * cxt, const void *nodep, const VISIT which, const int depth) {
  struct cee_tuple  * p;
  switch (which) 
  {
    case preorder:
    case leaf:
      p = (struct cee_tuple *)*(void **)nodep;
      cee_list_append((struct cee_list **)cxt, p->_[1]);
      break;
    default:
      break;
  }
}

struct cee_list * cee_map_values(struct cee_map * m) {
  uintptr_t s = cee_map_size(m);
  struct S(header) * b = FIND_HEADER(m);
  struct cee_list * values = cee_list_mk(b->cs.state, s);
  b->context = values;
  musl_twalk(&values, b->_[0], S(get_value));
  return values;
}
