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
  union {
    struct {
      enum cee_del_policy key;
      enum cee_del_policy val;
    } _;
    enum cee_del_policy a[2];
  } del_policies;

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
  
  m->del_policies._.key = o[0];
  m->del_policies._.val = o[1];
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

void* cee_map_add_e(struct cee_map * m, void * key, void * value,
		    void *ctx, void* (*merge)(void *ctx, void *old_value, void *new_value)) {
  struct S(header) * b = FIND_HEADER(m);

  struct cee_tuple *t, *t1 = NULL, **oldp;
  t = cee_tuple_mk_e(b->cs.state, b->del_policies.a, key, value);
  oldp = musl_tsearch(b, t, b->_, S(cmp));

  if (oldp == NULL)
    cee_segfault(); /* run out of memory */
  else if (*oldp != t) {
    t1 = *oldp;
    void *old_value = t1->_[1];
    void *new_value = value;
    if (merge)
      new_value = merge(ctx, old_value, new_value);

    /* detach old_value  and capture new_value */
    if (new_value != old_value) {
      t1->_[1] = new_value; 
      cee_decr_indegree(b->del_policies._.val, old_value); /* decrease the rc of old value */
    }
    cee_tuple_update_del_policy(t, 1, CEE_DP_NOOP); /* do nothing for t[1] */
    cee_del(t);    
    return old_value;
  }
  else
    b->size ++;
  return NULL;
}

void* cee_map_add(struct cee_map * m, void * key, void * value) {
  return cee_map_add_e(m, key, value, NULL, NULL);
}

void* cee_map_find(struct cee_map * m, void * key){
  if( key == NULL ) return NULL;
  struct S(header) * b = FIND_HEADER(m);
  struct cee_tuple t = { key, 0 };
  struct cee_tuple **pp = musl_tfind(b, &t, b->_, S(cmp));
  if( pp == NULL )
    return NULL;
  else{
    struct cee_tuple *p = *pp;
    return p->_[1];
  }
}

void * cee_map_remove(struct cee_map * m, void *key) {
  struct S(header) *b = FIND_HEADER(m);
  struct cee_tuple t = { key, 0 };
  struct cee_tuple **pp = musl_tfind(b, &t, b->_, S(cmp));
  if (pp == NULL)
    return NULL;
  else {
    b->size --;
    struct cee_tuple *old_t = *pp;
    musl_tdelete(b, &t, b->_, S(cmp));
    void *old_value = old_t->_[1];
    cee_decr_indegree(b->del_policies._.key, old_t->_[0]); /* decrease the rc of key */
    cee_decr_indegree(b->del_policies._.val, old_t->_[1]); /* decrease the rc of val */
    cee_tuple_update_del_policy(old_t, 0, CEE_DP_NOOP); /* do nothing for t[0] */
    cee_tuple_update_del_policy(old_t, 1, CEE_DP_NOOP); /* do nothing for t[1] */
    cee_del(old_t);
    return old_value;
  }
}

bool cee_map_rename(struct cee_map *m, void *old_key, void *new_key) {
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
  struct cee_list *keys = cee_list_mk(b->cs.state, n);
  musl_twalk(keys, b->_[0], S(get_key));
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
  musl_twalk(values, b->_[0], S(get_value));
  return values;
}


/*
 * internal structure for cee_map_iterate
 */
struct S(fn_ctx) {
  void *ctx;
  int (*f)(void *ctx, void *key, void *value);
  int ret;
};

/*
 * helper function for cee_map_iterate
 */
static void S(apply_each) (void *ctx, const void *nodep, const VISIT which, const int depth) {
  struct cee_tuple *p;
  struct S(fn_ctx) * fn_ctx_p = ctx;
  if (fn_ctx_p->ret) 
    /* the previous iteration has an error, skip the rest iterations */
    return;

  switch(which)
  {
  case preorder:
  case leaf:
    p = *(void **)nodep;
    fn_ctx_p->ret = fn_ctx_p->f(fn_ctx_p->ctx, p->_[0], p->_[1]);
    break;
  default:
    break;
  }
}

/*
 * iterate
 */
int cee_map_iterate(struct cee_map *m, void *ctx,
                    int (*f)(void *ctx, void *key, void *value))
{
  if (!m) return 0;
  struct S(header) *b = FIND_HEADER(m);
  struct S(fn_ctx) fn_ctx = { .ctx = ctx, .f = f, .ret = 0 };
  musl_twalk(&fn_ctx, b->_[0], S(apply_each));
  return fn_ctx.ret;
}


struct S(merge_ctx) {
  struct cee_map *dest_map;
  void *merge_ctx;
  void* (*merge)(void *ctx, void *old_value, void *new_value);
};

static int S(_add_kv)(void *ctx, void *key, void *value)
{
  struct S(merge_ctx) *mctx = ctx;
  cee_map_add_e(mctx->dest_map, key, value, mctx->merge_ctx, mctx->merge);
  return 0;
}

/*
 * add all key/value pairs of the src to the dest
 * and keep the src intact.
 */
void cee_map_merge(struct cee_map *dest, struct cee_map *src,
		   void *ctx, void* (*merge)(void *ctx, void *old, void *new))
{
  struct S(merge_ctx) mctx = { .dest_map = dest, .merge_ctx = ctx, .merge = merge };
  cee_map_iterate(src, &mctx, S(_add_kv));
}

struct cee_map* cee_map_clone(struct cee_map *src)
{
  struct S(header) *m = FIND_HEADER(src);
  struct cee_map *new_map = cee_map_mk_e(cee_get_state(src), m->del_policies.a, m->cmp);
  cee_map_merge(new_map, src, NULL, NULL);
  return new_map;
}
