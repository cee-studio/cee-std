#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_dict_##f
#else
#define  S(f)  _##f
#include "cee.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <errno.h>
#include "musl-search.h"
#endif
#include "cee-header.h"

struct S(header) {
  struct cee_list * keys;
  struct cee_list * vals;
  uintptr_t size;
  enum cee_del_policy del_policy;
  struct cee_sect cs;
  struct musl_hsearch_data _[1];
};

#include "cee-resize.h"

static void S(trace)(void *d, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(d);
  
  switch (ta) {
    case CEE_TRACE_DEL_NO_FOLLOW:
      musl_hdestroy_r(m->_);
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_DEL_FOLLOW:
      cee_del_e(m->del_policy, m->keys);
      cee_del_e(m->del_policy, m->vals);
      musl_hdestroy_r(m->_);
      S(de_chain)(m);
      free(m);
      break;
    default:
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      cee_trace(m->keys, ta);
      cee_trace(m->vals, ta);
      break;
  }
}

struct cee_dict * cee_dict_mk_e (struct cee_state * s, enum cee_del_policy o, size_t size) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * m = malloc(mem_block_size);
  m->del_policy = o;
  m->keys = cee_list_mk(s, size);
  cee_use_realloc(m->keys);
  
  m->vals = cee_list_mk(s, size);
  cee_use_realloc(m->vals);
  
  m->size = size;
  ZERO_CEE_SECT(&m->cs);
  S(chain)(m, s);
  
  m->cs.trace = S(trace);
  m->cs.mem_block_size = mem_block_size;
  m->cs.resize_method = CEE_RESIZE_WITH_IDENTITY;
  m->cs.n_product = 2; // key:str, value
  size_t  hsize = (size_t)((float)size * 1.25);
  memset(m->_, 0, sizeof(struct musl_hsearch_data));
  if (musl_hcreate_r(hsize, m->_)) {
    return (struct cee_dict *)(&m->_);
  }
  else {
    cee_del(m->keys);
    cee_del(m->vals);
    free(m);
    return NULL;
  }
}

struct cee_dict * cee_dict_mk (struct cee_state *s, size_t size) {
  return cee_dict_mk_e (s, CEE_DEFAULT_DEL_POLICY, size);
}

void cee_dict_add (struct cee_dict * d, char * key, void * value) {
  struct S(header) * m = FIND_HEADER(d);
  MUSL_ENTRY n, *np;
  n.key = key;
  n.data = value;
  if (!musl_hsearch_r(n, ENTER, &np, m->_))
    cee_segfault();
  cee_list_append(&m->keys, key);
  cee_list_append(&m->vals, value);
}

void * cee_dict_find(struct cee_dict * d, char * key) {
  struct S(header) * m = FIND_HEADER(d);
  MUSL_ENTRY n, *np;
  n.key = key;
  n.data = NULL;
  if (musl_hsearch_r(n, FIND, &np, m->_))
    return np->data;
  printf ("%s\n", strerror(errno));
  return NULL;
}
