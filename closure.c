#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_closure_##f
#else
#define  S(f)  _##f
#include "cee.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#endif
#include "cee-header.h"

struct S(header) {
  struct cee_sect cs;
  struct cee_closure _;
};
    
#include "cee-resize.h"    

static void S(trace) (void * v, enum cee_trace_action sa) {
  struct S(header) * m = FIND_HEADER(v);
  switch (sa) {
    case CEE_TRACE_DEL_NO_FOLLOW:
    case CEE_TRACE_DEL_FOLLOW:
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_MARK:
    default:
      break;
  }
}

struct cee_closure * cee_closure_mk (struct cee_state * s, struct cee_env * env, void * fun) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * b = malloc(mem_block_size);
  ZERO_CEE_SECT(&b->cs);
  S(chain)(b, s);
  
  b->cs.trace = S(trace);
  b->cs.resize_method = CEE_RESIZE_WITH_IDENTITY;
  b->cs.mem_block_size = mem_block_size;
  
  b->_.env = env;
  b->_.vfun = fun;
  return &(b->_);
}

void * cee_closure_call (struct cee_state * s, struct cee_closure * c, size_t n, ...) {
  va_list ap;
  va_start(ap, n);

  void *ret = c->vfun(s, c->env, n, ap);

  va_end(ap);

  return ret;
}
