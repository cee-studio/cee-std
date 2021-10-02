#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_env_##f
#else
#define  S(f)  _##f
#include "cee.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif
#include "cee-header.h"

struct S(header) {
  enum cee_del_policy env_dp;
  enum cee_del_policy vars_dp;
  struct cee_sect cs;
  struct cee_env _;
};
    
#include "cee-resize.h"
    
static void S(trace) (void * v, enum cee_trace_action ta) {
  struct S(header) * h = FIND_HEADER(v);
  switch (ta) {
    case CEE_TRACE_DEL_NO_FOLLOW:
      S(de_chain)(h);
      free(h);
      break;
    case CEE_TRACE_DEL_FOLLOW:
      cee_del_e(h->env_dp, h->_.outer);
      cee_del_e(h->vars_dp, h->_.vars);
      S(de_chain)(h);
      free(h);
      break;
    case CEE_TRACE_MARK:
    default:
      h->cs.gc_mark = ta - CEE_TRACE_MARK;
      cee_trace(h->_.outer, ta);
      cee_trace(h->_.vars, ta);
      break;
  }
}

struct cee_env * cee_env_mk_e(struct cee_state * st, enum cee_del_policy dp[2], struct cee_env * outer, struct cee_map * vars) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * h = malloc(mem_block_size);

  ZERO_CEE_SECT(&h->cs);
  S(chain)(h, st);
  
  h->cs.trace = S(trace);
  h->cs.resize_method = CEE_RESIZE_WITH_IDENTITY;
  h->cs.mem_block_size = mem_block_size;
  h->cs.cmp = NULL;
  h->cs.n_product = 0;
  
  h->env_dp = dp[0];
  h->vars_dp = dp[1];
  h->_.outer = outer;
  h->_.vars = vars;
  
  return &h->_;
}
 
struct cee_env * cee_env_mk(struct cee_state * st, struct cee_env * outer, struct cee_map * vars) {
  enum cee_del_policy dp[2] = { CEE_DP_DEL_RC, CEE_DP_DEL_RC };
  return cee_env_mk_e (st, dp, outer, vars);
}

void * cee_env_find(struct cee_env * e, char * key) {
  struct cee_boxed * ret;
  while (e && e->vars) {
    if ( (ret = cee_map_find(e->vars, key)) ) 
      return ret;
    e = e->outer;
  }
  return NULL;
}
