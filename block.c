#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_block_##f
#else
#define  S(f)  _##f
#include "cee.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#endif
#include "cee-header.h"

#ifndef CEE_BLOCK
#define  CEE_BLOCK 64
#endif

struct S(header) {
  uintptr_t capacity;
  enum cee_del_policy del_policy;
  struct cee_sect cs;
  char _[1]; /* actual data */
};

#include "cee-resize.h"

static void S(trace) (void * p, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(p);
  switch (ta) {
    case CEE_TRACE_DEL_FOLLOW:
    case CEE_TRACE_DEL_NO_FOLLOW:
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_MARK:
    default:
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      break;
  } 
}
    
static void S(mark) (void * p) {
  /* we don't know anything about this block 
   * do nothing now. 
   */
};

void *cee_block_mk_nonzero(struct cee_state *s, size_t n){
  size_t mem_block_size;
  va_list ap;
  
  mem_block_size = n + sizeof(struct S(header));
  struct S(header) * m = malloc(mem_block_size);
  
  ZERO_CEE_SECT(&m->cs);
  m->del_policy = CEE_DP_DEL_RC;
  S(chain)(m, s);
  
  m->cs.trace = S(trace);
  m->cs.resize_method = CEE_RESIZE_WITH_MALLOC;
  m->cs.mem_block_size = mem_block_size;
  m->cs.cmp = (void *)memcmp;
  m->capacity = n;
  
  return (struct cee_block *)(m->_);
}

void *cee_block_mk(struct cee_state *s, size_t n){
  void *p = cee_block_mk_nonzero(s, n);
  memset(p, 0, n);
  return p;
}

/*
 * @param init_f: a function to initialize the allocated block
 */
void *cee_block_mk_e(struct cee_state *s, size_t n, void *cxt, void (*init_f)(void *cxt, void *block)){
  void *block = cee_block_mk(s, n);
  init_f(cxt, block);
  return block;
}


size_t cee_block_size (struct cee_block *b)
{
  struct S(header) *h = FIND_HEADER(b);
  return h->capacity;
}
