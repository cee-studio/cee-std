#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_tagged_##f
#else
#define  S(f)  _##f
#include "cee.h"
#include <stdlib.h>
#include <string.h>
#endif
#include "cee-header.h"

struct S(header) {
  enum cee_del_policy del_policy;
  struct cee_sect cs;
  struct cee_tagged _;
};
    
#include "cee-resize.h"    

static void S(trace) (void * v, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(v);
  switch (ta) {
    case CEE_TRACE_DEL_NO_FOLLOW:
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_DEL_FOLLOW:
      cee_del_e(m->del_policy, m->_.ptr._);
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_MARK:
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      cee_trace(m->_.ptr._, ta);
      break;
  }
}

struct cee_tagged * cee_tagged_mk_e (struct cee_state * st, enum cee_del_policy o, uintptr_t tag, void *p) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * b = malloc(mem_block_size);
  ZERO_CEE_SECT(&b->cs);
  S(chain)(b, st);
  
  b->cs.trace = S(trace);
  b->cs.resize_method = CEE_RESIZE_WITH_IDENTITY;
  b->cs.mem_block_size = mem_block_size;
  
  b->_.tag = tag;
  b->_.ptr._ = p;
  b->del_policy = o;
  cee_incr_indegree(o, p);
  return &b->_;
}

struct cee_tagged * cee_tagged_mk (struct cee_state * st, uintptr_t tag, void *p) {
  return cee_tagged_mk_e(st, CEE_DP_DEL_RC, tag, p);
}
