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
  void * _[];
};

#include "cee-resize.h"

static void S(trace) (void * v, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(v);
  int i;
  
  switch(ta) {
    case CEE_TRACE_DEL_NO_FOLLOW:
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_DEL_FOLLOW:
      for (i = 0; i < m->size; i++)
        cee_del_e(m->del_policy, m->_[i]);
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_MARK:
    default:
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      for (i = 0; i < m->size; i++)
        cee_trace(m->_[i], ta);
      break;
  }
}

struct cee_list * cee_list_mk_e (struct cee_state * st, enum cee_del_policy o, size_t cap) {
  size_t mem_block_size = sizeof(struct S(header)) + cap * sizeof(void *);
  struct S(header) * m = malloc(mem_block_size);
  m->capacity = cap;
  m->size = 0;
  m->del_policy = o;
  ZERO_CEE_SECT(&m->cs);
  S(chain)(m, st);
  
  m->cs.trace = S(trace);
  m->cs.resize_method = CEE_RESIZE_WITH_MALLOC;
  m->cs.mem_block_size = mem_block_size;
  
  return (struct cee_list *)(m->_);
}

struct cee_list * cee_list_mk (struct cee_state * s, size_t cap) {
  return cee_list_mk_e(s, CEE_DP_DEL_RC, cap);
}

struct cee_list * cee_list_append (struct cee_list ** l, void *e) {
  struct cee_list * v = *l;
    
  struct S(header) * m = FIND_HEADER(v);
  size_t capacity = m->capacity;
  size_t extra_cap = capacity ? capacity : 1;
  if (m->size == m->capacity) {
    size_t new_mem_block_size = m->cs.mem_block_size + extra_cap * sizeof(void *);
    struct S(header) * m1 = S(resize)(m, new_mem_block_size);
    m1->capacity = capacity + extra_cap;
    *l = (struct cee_list *)m1->_;
    m = m1;
  }
  m->_[m->size] = e;
  m->size ++;
  cee_incr_indegree(m->del_policy, e);
  return *l;
}

struct cee_list * cee_list_insert(struct cee_state * s, struct cee_list ** l, int index, void *e) {
  struct cee_list * v = *l;
  if (v == NULL) {
    v = cee_list_mk(s, 10);
    cee_use_realloc(v);
  }
  
  struct S(header) * m = FIND_HEADER(v);
  size_t capacity = m->capacity;
  size_t extra_cap = capacity ? capacity : 1;
  if (m->size == m->capacity) {
    size_t new_mem_block_size = m->cs.mem_block_size + extra_cap * sizeof(void *);
    struct S(header) * m1 = S(resize)(m, new_mem_block_size);
    m1->capacity = capacity + extra_cap;
    *l = (struct cee_list *)m1->_;
    m = m1;
  }
  int i;
  for (i = m->size; i > index; i--)
    m->_[i] = m->_[i-1];
  
  m->_[index] = e;
  m->size ++;
  cee_incr_indegree(m->del_policy, e);
  return *l;
}

bool cee_list_remove(struct cee_list * v, int index) {
  struct S(header) * m = FIND_HEADER(v);
  if (index >= m->size) return false;
 
  void * e = m->_[index];
  m->_[index] = 0;
  int i;
  for (i = index; i < (m->size - 1); i++)
    m->_[i] = m->_[i+1];
  
  m->size --;
  cee_decr_indegree(m->del_policy, e);
  return true;
}


size_t cee_list_size (struct cee_list *x) {
  struct S(header) * m = FIND_HEADER(x);
  return m->size;
}

size_t cee_list_capacity (struct cee_list *x) {
  struct S(header) * h = FIND_HEADER(x);
  return h->capacity;
}

void cee_list_iterate (struct cee_list *x, void *ctx,
		       void (*f)(void *cxt, int idx, void *e)) {
  struct S(header) *m = FIND_HEADER(x);
  int i;
  for (i = 0; i < m->size; i++)
    f(ctx, i, m->_[i]);
  return;
}
