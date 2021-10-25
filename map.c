#ifdef CEE_AMALGAMATION
#undef  S
#define S(f)  _cee_map_##f
#else
#define S(f)  _##f
#include "cee.h"
#include "musl-search.h"
#include <stdlib.h>
#include <string.h>
#endif
#include "cee-header.h"


struct S(header) {
  int (*cmp)(const void *l, const void *r);
  uintptr_t size;
  enum cee_del_policy key_del_policy;
  enum cee_del_policy val_del_policy;
  enum cee_trace_action ta;
  struct cee_sect cs;
  void * _[1];
};

#include "cee-resize.h"

static void S(free_pair_follow)(void * ctx, void * c) {
  cee_del(c);
}
        
static void S(trace_pair) (void * ctx, const void *nodep, const VISIT which, const int depth) {
  struct cee_tuple * p;
  struct S(header) * h;
  switch (which) 
  {
    case preorder:
    case leaf:
      p = *(void **)nodep;
      cee_trace(p, *(enum cee_trace_action *)ctx);
      break;
    default:
      break;
  }
}

static void S(trace)(void * p, enum cee_trace_action ta) {
  struct S(header) * h = FIND_HEADER (p);
  switch (ta) {
    case CEE_TRACE_DEL_NO_FOLLOW:
      musl_tdestroy(NULL, h->_[0], NULL);
      S(de_chain)(h);
      free(h);
      break;
    case CEE_TRACE_DEL_FOLLOW:
      musl_tdestroy(&ta, h->_[0], S(free_pair_follow));
      S(de_chain)(h);
      free(h);
      break;
    case CEE_TRACE_MARK:
    default:
      h->cs.gc_mark = ta - CEE_TRACE_MARK;
      h->ta = ta;
      musl_twalk(&ta, h->_[0], S(trace_pair));
      break;
  }
}

static int S(cmp) (void * ctx, const void * v1, const void * v2) {
  struct S(header) * h = ctx;
  struct cee_tuple * t1 = (void *)v1; /* to remove const */
  struct cee_tuple * t2 = (void *)v2;
  return h->cmp(t1->_[0], t2->_[0]);
}

struct cee_map * cee_map_mk_e (struct cee_state * st, enum cee_del_policy o[2], 
                  int (*cmp)(const void *, const void *)) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * m = malloc(mem_block_size);
  m->cmp = cmp;
  m->size = 0;
  ZERO_CEE_SECT(&m->cs);
  S(chain)(m, st);
  
  m->cs.trace = S(trace);
  m->cs.resize_method = CEE_RESIZE_WITH_IDENTITY;
  m->cs.mem_block_size = mem_block_size;
  m->cs.cmp = 0;
  m->cs.cmp_stop_at_null = 0;
  m->cs.n_product = 2; /* key, value */
  
  m->key_del_policy = o[0];
  m->val_del_policy = o[1];
  m->_[0] = 0;
  return (void *)m->_;
}

struct cee_map * cee_map_mk(struct cee_state * st, int (*cmp) (const void *, const void *)) {
  static enum cee_del_policy d[2] = { CEE_DP_DEL_RC, CEE_DP_DEL_RC };
  return cee_map_mk_e(st, d, cmp);
}

uintptr_t cee_map_size(struct cee_map * m) {
  if (!m) return 0;
  struct S(header) * b = FIND_HEADER(m);
  return b->size;
}

void* cee_map_add(struct cee_map * m, void * key, void * value) {
  struct S(header) * b = FIND_HEADER(m);

  enum cee_del_policy d[2];
  d[0] = b->key_del_policy;
  d[1] = b->val_del_policy;
  
  struct cee_tuple *t, *t1 = NULL, **oldp;
  t = cee_tuple_mk_e(b->cs.state, d, key, value);
  oldp = musl_tsearch(b, t, b->_, S(cmp));

  if (oldp == NULL)
    cee_segfault(); /* run out of memory */
  else if (*oldp != t) {
    t1 = *oldp;
    void *old_value = t1->_[1];
    t1->_[1] = value; /* detach old value  and capture value */
    cee_decr_indegree(d[1], old_value); /* decrease the rc of old value */
    cee_tuple_update_del_policy(t, 1, CEE_DP_NOOP); /* do nothing for t[1] */
    cee_del(t);
    return old_value;
  }
  else
    b->size ++;
  return NULL;
}

void * cee_map_find(struct cee_map * m, void * key) {
  struct S(header) * b = FIND_HEADER(m);
  struct cee_tuple t = { key, 0 };
  struct cee_tuple **pp = musl_tfind(b, &t, b->_, S(cmp));
  if (pp == NULL)
    return NULL;
  else {
    struct cee_tuple *p = *pp;
    return p->_[1];
  }
}

void * cee_map_remove(struct cee_map * m, void * key) {
  struct S(header) *b = FIND_HEADER(m);
  void **oldp = musl_tdelete(b, key, b->_, S(cmp));
  if (oldp == NULL)
    return NULL;
  else {
    b->size --;
    struct cee_tuple *ret = *oldp;
    cee_del(ret);
    cee_decr_indegree(b->key_del_policy, ret->_[0]);
    cee_decr_indegree(b->val_del_policy, ret->_[1]);
    return ret->_[1];
  }
}

bool cee_map_replace(struct cee_map *m, void *old_key, void *new_key) {
  void *v = cee_map_remove(m, old_key);
  if (v) {
    cee_map_add(m, new_key, v);
    return true;
  }
  else
    return false;
}

static void S(get_key) (void * ctx, const void *nodep, const VISIT which, const int depth) {
  struct cee_tuple * p;
  switch (which) 
  {
    case preorder:
    case leaf:
      p = *(void **)nodep;
      cee_list_append(ctx, p->_[0]);
      break;
    default:
      break;
  }
}

struct cee_list * cee_map_keys(struct cee_map * m) {
  uintptr_t n = cee_map_size(m);
  struct S(header) * b = FIND_HEADER(m);
  struct cee_list * keys = cee_list_mk(b->cs.state, n);
  musl_twalk(&keys, b->_[0], S(get_key));
  return keys;
}


static void S(get_value) (void * ctx, const void *nodep, const VISIT which, const int depth) {
  struct cee_tuple  * p;
  switch (which) 
  {
    case preorder:
    case leaf:
      p = *(void **)nodep;
      cee_list_append(ctx, p->_[1]);
      break;
    default:
      break;
  }
}

struct cee_list* cee_map_values(struct cee_map *m) {
  uintptr_t s = cee_map_size(m);
  struct S(header) *b = FIND_HEADER(m);
  struct cee_list *values = cee_list_mk(b->cs.state, s);
  musl_twalk(&values, b->_[0], S(get_value));
  return values;
}


/*
 * internal structure for cee_map_iterate
 */
struct S(fn_ctx) {
  void *ctx;
  void (*f)(void *ctx, void *key, void *value);
};

/*
 * helper function for cee_map_iterate
 */
static void S(apply_each) (void *ctx, const void *nodep, const VISIT which, const int depth) {
  struct cee_tuple *p;
  struct S(fn_ctx) * fn_ctx_p = ctx;
  switch(which)
  {
  case preorder:
  case leaf:
    p = *(void **)nodep;
    fn_ctx_p->f(fn_ctx_p->ctx, p->_[0], p->_[1]);
    break;
  default:
    break;
  }
}

/*
 * iterate
 */
void cee_map_iterate(struct cee_map *m, void *ctx,
                     void (*f)(void *ctx, void *key, void *value))
{
  if (!m) return;
  struct S(header) *b = FIND_HEADER(m);
  struct S(fn_ctx) fn_ctx = { .ctx = ctx, .f = f };
  musl_twalk(&fn_ctx, b->_[0], S(apply_each));
  return;
}
