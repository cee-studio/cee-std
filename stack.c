#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)   _cee_stack_##f
#else
#define  S(f)   _##f
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "cee.h"
#endif
#include "cee-header.h"

struct S(header) {
  uintptr_t used;
  uintptr_t top;
  uintptr_t capacity;
  enum cee_del_policy del_policy;
  struct cee_sect cs;
  void * _[];
};
    
#include "cee-resize.h"    

static void S(trace) (void * v, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(v);
  int i;
  
  switch (ta) {
    case CEE_TRACE_DEL_NO_FOLLOW:
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_DEL_FOLLOW:
      for (i = 0; i < m->used; i++)
        cee_del_e(m->del_policy, m->_[i]);
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_MARK:
    default:
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      for (i = 0; i < m->used; i++)
        cee_trace(m->_[i], ta);
      break;
  }
}

struct cee_stack * cee_stack_mk_e (struct cee_state * st, enum cee_del_policy o, size_t size) {
  uintptr_t mem_block_size = sizeof(struct S(header)) + size * sizeof(void *);
  struct S(header) * m = malloc(mem_block_size);
  m->capacity = size;
  m->used = 0;
  m->top = (0-1);
  m->del_policy = o;
  
  ZERO_CEE_SECT(&m->cs);
  S(chain)(m, st);
  
  m->cs.trace = S(trace);
  m->cs.mem_block_size = mem_block_size;
  return (struct cee_stack *)(m->_);
}

struct cee_stack * cee_stack_mk (struct cee_state * st, size_t size) {
  return cee_stack_mk_e(st, CEE_DP_DEL_RC, size);
}

int cee_stack_push (struct cee_stack * v, void *e) {
  struct S(header) * m = FIND_HEADER((void **)v);
  if (m->used == m->capacity)
    return 0;

  m->top ++;
  m->used ++;
  m->_[m->top] = e;
  cee_incr_indegree(m->del_policy, e);
  return 1;
}

void * cee_stack_pop (struct cee_stack * v) {
  struct S(header) * b = FIND_HEADER((void **)v);
  if (b->used == 0) {
    return NULL;
  } 
  else {
    void * p = b->_[b->top];
    b->used --;
    b->top --;
    cee_decr_indegree(b->del_policy, p);
    return p;
  }
}

/*
 *  nth: 0 -> the topest element
 *       1 -> 1 element way from the topest element
 */
void * cee_stack_top (struct cee_stack * v, uintptr_t nth) {
  struct S(header) * b = FIND_HEADER(v);
  if (b->used == 0 || nth >= b->used)
    return NULL;
  else
    return b->_[b->top-nth];
}

uintptr_t cee_stack_size (struct cee_stack *x) {
  struct S(header) * m = FIND_HEADER((void **)x);
  return m->used;
}

#if 0
uintptr_t cee_stack_capacity (struct cee_stack *s) {
  struct S(header) * m = FIND_HEADER(s);
  return m->capacity;
}
#endif

bool cee_stack_empty (struct cee_stack *x) {
  struct S(header) * b = FIND_HEADER(x);
  return b->used == 0;
}

bool cee_stack_full (struct cee_stack *x) {
  struct S(header) * b = FIND_HEADER(x);
  return b->used >= b->capacity;
}
