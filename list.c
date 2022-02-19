#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_list_##f
#else
#define  S(f)  _##f
#include "cee.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#endif
#include "cee-header.h"

struct S(header) {
  uintptr_t size;
  uintptr_t capacity;
  enum cee_del_policy del_policy;
  struct cee_sect cs;
  struct cee_list _;
};

#include "cee-resize.h"

static void S(trace) (void * v, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(v);
  int i;
  
  switch(ta) {
    case CEE_TRACE_DEL_NO_FOLLOW:
      S(de_chain)(m);
      free(m->_.a);
      free(m);
      break;
    case CEE_TRACE_DEL_FOLLOW:
      for (i = 0; i < m->size; i++)
        cee_del_e(m->del_policy, m->_.a[i]);
      S(de_chain)(m);
      free(m->_.a);
      free(m);
      break;
    case CEE_TRACE_MARK:
    default:
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      for (i = 0; i < m->size; i++)
        cee_trace(m->_.a[i], ta);
      break;
  }
}

struct cee_list * cee_list_mk_e (struct cee_state * st, enum cee_del_policy o, size_t cap) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * m = malloc(mem_block_size);
  m->capacity = cap;
  m->size = 0;
  m->del_policy = o;
  ZERO_CEE_SECT(&m->cs);
  S(chain)(m, st);
  
  m->cs.trace = S(trace);
  m->cs.resize_method = CEE_RESIZE_WITH_MALLOC;
  m->cs.mem_block_size = mem_block_size;

  struct cee_list *ret = (struct cee_list *)&(m->_);
  ret->a = malloc(cap * sizeof(void *));
  
  return ret;
}

struct cee_list * cee_list_mk (struct cee_state * s, size_t cap) {
  return cee_list_mk_e(s, CEE_DP_DEL_RC, cap);
}

void cee_list_append (struct cee_list *v, void *e) {
  struct S(header) * m = FIND_HEADER(v);
  size_t capacity = m->capacity;
  size_t extra_cap = capacity ? capacity : 1;
  if (m->size == m->capacity) {
    m->capacity = capacity + extra_cap;    
    void *a = realloc(v->a, m->capacity * sizeof(void *));
    v->a = a;
  }
  v->a[m->size] = e;
  m->size ++;
  cee_incr_indegree(m->del_policy, e);
}

void cee_list_insert(struct cee_list *v, int index, void *e) {
  struct S(header) * m = FIND_HEADER(v);
  size_t capacity = m->capacity;
  size_t extra_cap = capacity ? capacity : 1;
  if (m->size == m->capacity) {
    m->capacity = capacity + extra_cap;
    void *a = realloc(v->a, m->capacity * sizeof(void *));
    v->a = a;
  }
  int i;
  for (i = m->size; i > index; i--)
    v->a[i] = v->a[i-1];
  
  v->a[index] = e;
  m->size ++;
  cee_incr_indegree(m->del_policy, e);
}

bool cee_list_remove(struct cee_list *v, int index) {
  if (!v) return false;
  struct S(header) *m = FIND_HEADER(v);
  if (index >= m->size) return false;
 
  void *e = v->a[index];
  v->a[index] = 0;
  int i;
  for (i = index; i < (m->size - 1); i++)
    v->a[i] = v->a[i+1];
  
  m->size --;
  cee_decr_indegree(m->del_policy, e);
  return true;
}


size_t cee_list_size (struct cee_list *x) {
  if (!x) return 0;
  struct S(header) * m = FIND_HEADER(x);
  return m->size;
}

size_t cee_list_capacity (struct cee_list *x) {
  if (!x) return 0;
  struct S(header) * h = FIND_HEADER(x);
  return h->capacity;
}

int cee_list_iterate (struct cee_list *x, void *ctx,
		       int (*f)(void *cxt, int idx, void *e)) {
  if (!x) return 0;
  struct S(header) *m = FIND_HEADER(x);
  int i, ret;
  for (i = 0; i < m->size; i++) {
    ret = f(ctx, i, x->a[i]);
    if (ret) break;
  }
  return ret;
}

static int S(_add_v)(void *cxt, int idx, void *e) {
  struct cee_list *l = cxt;
  cee_list_append(l, e);
  return 0;
}

void cee_list_merge (struct cee_list *dest, struct cee_list *src)
{
  cee_list_iterate (src, dest, S(_add_v));
}
