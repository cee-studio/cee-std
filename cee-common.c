#include "cee.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cee-internal.h"

#define FIND_SECT(p)  (struct cee_sect *)((void *)((char *)p - sizeof(struct cee_sect)))

void cee_trace (void *p, enum cee_trace_action ta) {
  if (!p) return;
  
  struct cee_sect * cs = FIND_SECT(p);
  cs->trace(p, ta);
}

/*
 * a generic resource delete function for all cee_* pointers
 */
void cee_del(void *p) {
  if (!p) return;
  
  struct cee_sect * cs = FIND_SECT(p);
  cs->trace(p, CEE_TRACE_DEL_FOLLOW);
}

void cee_del_ref(void *p) {
  if (!p) return;
  
  struct cee_sect * cs = FIND_SECT(p);
  
  if (cs->in_degree) cs->in_degree --;
  
  /* if it's retained by an owner,
     it should be freed by cee_del
  */
  if (cs->retained) return;
  
  if (!cs->in_degree) cs->trace(p, CEE_TRACE_DEL_FOLLOW);
}

void cee_use_realloc(void * p) {
  struct cee_sect * cs = FIND_SECT(p);
  if (cs->resize_method)
    cs->resize_method = CEE_RESIZE_WITH_REALLOC;
}

void cee_use_malloc(void * p) {
  struct cee_sect * cs = FIND_SECT(p);
  if (cs->resize_method)
    cs->resize_method = CEE_RESIZE_WITH_MALLOC;
}

void cee_segfault() {
  volatile char * c = 0;
  *c = 0;
  __builtin_unreachable();
}

static void _cee_common_incr_rc (void * p) {
  struct cee_sect * cs = FIND_SECT(p);
  if (cs->retained) return;
  
  cs->in_degree ++;
}

static void _cee_common_decr_rc (void * p) {
  struct cee_sect * cs = FIND_SECT(p);
  if (cs->retained) return;
  
  if (cs->in_degree)
    cs->in_degree --;
  else {
    // report warnings
  }
}

uint16_t get_in_degree (void * p) {
  struct cee_sect * cs = FIND_SECT(p);
  return cs->in_degree;
}

static void _cee_common_retain (void *p) {
  struct cee_sect * cs = FIND_SECT(p);
  cs->retained = 1;
}

static void _cee_common_release (void * p) {
  struct cee_sect * cs = FIND_SECT(p);
  if(cs->retained)
    cs->retained = 0;
  else {
    // report error
    cee_segfault();
  }
}

void cee_incr_indegree (enum cee_del_policy o, void * p) {
  switch(o) {
    case CEE_DP_DEL_RC:
      _cee_common_incr_rc(p);
      break;
    case CEE_DP_DEL:
      _cee_common_retain(p);
      break;
    case CEE_DP_NOOP:
      break;
  }
}

void cee_decr_indegree (enum cee_del_policy o, void * p) {
  switch(o) {
    case CEE_DP_DEL_RC:
      _cee_common_decr_rc(p);
      break;
    case CEE_DP_DEL:
      _cee_common_release(p);
      break;
    case CEE_DP_NOOP:
      break;
  }
}


void cee_del_e (enum cee_del_policy o, void *p) {
  switch(o) {
    case CEE_DP_DEL_RC:
      cee_del_ref(p);
      break;
    case CEE_DP_DEL:
      cee_del(p);
      break;
    case CEE_DP_NOOP:
      break;
  }
}
