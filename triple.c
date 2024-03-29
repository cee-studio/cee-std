#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_triple_##f
#else
#define  S(f)  _##f
#include "cee.h"
#include <stdlib.h>
#include <string.h>
#endif
#include "cee-header.h"

struct S(header) {
  enum cee_del_policy del_policies[3];
  struct cee_sect cs;
  void * _[3];
};
    
#include "cee-resize.h"
    
static void S(trace)(void * v, enum cee_trace_action ta) {
  struct S(header) * b = FIND_HEADER(v);
  int i;
  
  switch (ta) {
    case CEE_TRACE_DEL_NO_FOLLOW:
      S(de_chain)(b);
      free(b);
      break;
    case CEE_TRACE_DEL_FOLLOW:
      for (i = 0; i < 3; i++)
        cee_del_e(b->del_policies[i], b->_[i]);
      S(de_chain)(b);
      free(b);
      break;
    case CEE_TRACE_MARK:
    default:
      b->cs.gc_mark = ta - CEE_TRACE_MARK;
      for (i = 0; i < 3; i++)
        cee_trace(b->_[i], ta);
      break;
  }
}

struct cee_triple * cee_triple_mk_e (struct cee_state * st, enum cee_del_policy o[3], void * v1, void * v2, void * v3) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * m = malloc(mem_block_size);
  ZERO_CEE_SECT(&m->cs);
  S(chain)(m, st);
  
  m->cs.trace = S(trace);
  m->cs.resize_method = CEE_RESIZE_WITH_IDENTITY;
  m->cs.mem_block_size = mem_block_size;
  m->cs.state = st;
  m->_[0] = v1;
  m->_[1] = v2;
  m->_[2] = v3;
  int i;
  for (i = 0; i < 3; i++) {
    m->del_policies[i] = o[i];
    cee_incr_indegree(o[i], m->_[i]);
  }
  return (struct cee_triple *)&m->_;
}

struct cee_triple * cee_triple_mk (struct cee_state * st, void * v1, void * v2, void *v3) {
  static enum cee_del_policy o[3] = { CEE_DP_DEL_RC, CEE_DP_DEL_RC, CEE_DP_DEL_RC };
  return cee_triple_mk_e(st, o, v1, v2, v3);
}
