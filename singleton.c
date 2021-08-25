#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_singleton_##f
#else
#define  S(f)  _##f
#include <string.h>
#include "cee.h"
#include "cee-internal.h"
#endif
#include "cee-header.h"

struct S(header) {
  struct cee_sect cs;
  uintptr_t _; // tag
  uintptr_t val;
};

/*
 * the parameter of this function has to be a global/static 
 * uintptr_t array of two elements
 */
#if 1 // original
/*
 * singleton should never be deleted, hence we pass a noop
 */
static void S(noop)(void *p, enum cee_trace_action ta) {}

struct cee_singleton * cee_singleton_init(void *s, uintptr_t tag, uintptr_t val) {
  struct S(header) * b = (struct S(header)* )s;
  ZERO_CEE_SECT(&b->cs);
  b->cs.trace = S(noop);
  b->cs.resize_method = CEE_RESIZE_WITH_IDENTITY;
  b->cs.mem_block_size = 0;
  b->cs.n_product = 0;
  b->_ = tag;
  b->val = val;
  return (struct cee_singleton *)&(b->_);
}
#else // update attempt

#include "cee-resize.h"

static void S(trace) (void * v, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(v);
  switch(ta) {
    case CEE_TRACE_DEL_FOLLOW:
    case CEE_TRACE_DEL_NO_FOLLOW:
      S(de_chain)(m);
      free(m);
      break;
    default:
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      break;
  }
}

struct cee_singleton * cee_singleton_mk(struct cee_state * st, cee_tag_t tag, uintptr_t val) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * b = malloc(mem_block_size);
  ZERO_CEE_SECT(&b->cs);
  S(chain)(b, st);

  b->cs.trace = S(trace);
  b->cs.resize_method = CEE_RESIZE_WITH_IDENTITY;
  b->cs.mem_block_size = mem_block_size;

  b->_ = tag;
  b->val = val;
  cee_state_add_singleton(st, tag, val);
  return (struct cee_singleton *)&(b->_);
}
#endif
