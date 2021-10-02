#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_state_##f
#else
#define  S(f)  _##f
#include "cee.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif
#include "cee-header.h"

struct S(header) {
  struct cee_sect cs;
  struct cee_state _;
};

static void S(trace) (void * v, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(v);
  switch (ta) {
    case CEE_TRACE_DEL_FOLLOW: 
    {
      /* following this tracing chain but not the relations */
      struct cee_sect * tail = m->_.trace_tail;
      while (tail != &m->cs) {
        cee_trace(tail + 1, CEE_TRACE_DEL_NO_FOLLOW);
        tail = m->_.trace_tail;
      }
      free(m);
      break;
    }
    case CEE_TRACE_DEL_NO_FOLLOW: 
    {
      /* TODO detach the this state from all memory blocks */
      free(m);
      break;
    }
    case CEE_TRACE_MARK:
    default:
    {
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      cee_trace(m->_.roots, ta);
      cee_trace(m->_.stack, ta);
      cee_trace(m->_.contexts, ta);
      break;
    }
  }
}

static void S(sweep) (void * v, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(v);
  struct cee_sect * head  = &m->cs;
  while (head != NULL) {
    struct cee_sect * next = head->trace_next;
    if (head->gc_mark != ta - CEE_TRACE_MARK)
      cee_trace(head + 1, CEE_TRACE_DEL_NO_FOLLOW);
    head = next;
  }
}

static int S(cmp) (const void * v1, const void * v2) {
  printf("%p %p\n", v1, v2);
  if (v1 < v2)
    return -1;
  else if (v1 == v2)
    return 0;
  else
    return 1;
}

struct cee_state * cee_state_mk(size_t n) {
  size_t memblock_size = sizeof(struct S(header));
  struct S(header) * h = malloc(memblock_size);
  ZERO_CEE_SECT(&h->cs);
  h->cs.trace = S(trace);
  h->_.trace_tail = &h->cs; /* points to self; */
  
  struct cee_set * roots = cee_set_mk_e(&h->_, CEE_DP_NOOP, S(cmp));
  h->_.roots = roots;
  h->_.next_mark = 1;
  h->_.stack = cee_stack_mk(&h->_, n);
  h->_.contexts = cee_map_mk(&h->_, (cee_cmp_fun)strcmp);
  return &h->_;
}
  
void cee_state_add_gc_root(struct cee_state * s, void * key) {
  cee_set_add(s->roots, key);
}
  
void cee_state_remove_gc_root(struct cee_state * s, void * key) {
  cee_set_remove(s->roots, key);
}
  
void cee_state_add_context (struct cee_state * s, char * key, void * val) {
  cee_map_add(s->contexts, key, val);
}
  
void cee_state_remove_context (struct cee_state * s, char * key) {
  cee_map_remove(s->contexts, key);
}
  
void * cee_state_get_context (struct cee_state * s, char * key) {
  return cee_map_find(s->contexts, key);
}
  
void cee_state_gc (struct cee_state * s) {
  struct S(header) * h = FIND_HEADER(s);
  int mark = CEE_TRACE_MARK + s->next_mark;
  
  cee_trace(s, (enum cee_trace_action)mark);
  S(sweep)(s, (enum cee_trace_action) mark);
  
  if (s->next_mark == 0) {
    s->next_mark = 1;
  } else {
    s->next_mark = 0;
  }
}
