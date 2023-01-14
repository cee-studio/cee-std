#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_str_##f
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
  struct cee_sect cs;
  char _[1];
};

#include "cee-resize.h"

#undef _CEE_NEWSIZE
#define _CEE_NEWSIZE(c, extra)    (2 * ((c) > (extra) ? (c): (extra)))

static void S(trace) (void * p, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(p);
  switch (ta) {
    case CEE_TRACE_DEL_NO_FOLLOW:
    case CEE_TRACE_DEL_FOLLOW:
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_MARK:
    default:
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      break;
  }
}

struct cee_str * cee_str_mkv (struct cee_state *st, const char *fmt, va_list ap) {
  if (!fmt) {
    /* fmt cannot be null */
    /* intentionally cause a segfault */
    cee_segfault();
  }

  uintptr_t s;
  va_list saved_ap;
  va_copy(saved_ap, ap);

  s = vsnprintf(NULL, 0, fmt, ap);
  s ++;

  s += sizeof(struct S(header));
  s = (s / CEE_BLOCK + 1) * CEE_BLOCK;
  size_t mem_block_size = s;
  struct S(header) * h = malloc(mem_block_size);

  ZERO_CEE_SECT(&h->cs);
  S(chain)(h, st);
  
  h->cs.trace = S(trace);
  h->cs.resize_method = CEE_RESIZE_WITH_MALLOC;
  h->cs.mem_block_size = mem_block_size;
  h->cs.cmp = (void *)strcmp;
  h->cs.cmp_stop_at_null = 1;
  h->cs.n_product = 0;
  
  h->capacity = s - sizeof(struct S(header));

  vsnprintf(h->_, s, fmt, saved_ap);
  return (struct cee_str *)(h->_);
}

struct cee_str * cee_str_mk (struct cee_state * st, const char * fmt, ...) {
  if (!fmt) {
    /* fmt cannot be null */
    /* intentionally cause a segfault */
    cee_segfault();
  }

  va_list ap;

  va_start(ap, fmt);
  void *p = cee_str_mkv (st, fmt, ap);
  va_end(ap);
  return p;
}

struct cee_str * cee_str_mk_e (struct cee_state * st, size_t n, const char * fmt, ...) {
  uintptr_t s;
  va_list ap;

  if (fmt) {
    va_start(ap, fmt);
    s = vsnprintf(NULL, 0, fmt, ap);
    s ++; /* including the null terminator */
  }
  else
    s = n;

  s += sizeof(struct S(header));
  size_t mem_block_size = (s / CEE_BLOCK + 1) * CEE_BLOCK;
  struct S(header) * m = malloc(mem_block_size);

  ZERO_CEE_SECT(&m->cs);
  m->cs.trace = S(trace);
  m->cs.resize_method = CEE_RESIZE_WITH_MALLOC;
  m->cs.mem_block_size = mem_block_size;
  m->cs.cmp = (void *)strcmp;
  m->cs.cmp_stop_at_null = 1;
  
  S(chain)(m, st);
  
  m->capacity = m->cs.mem_block_size - sizeof(struct S(header));
  if( fmt ){
    va_start(ap, fmt);
    vsnprintf(m->_, m->capacity, fmt, ap);
  }else{
    m->_[0] = '\0'; /* terminates with '\0' */
  }
  return (struct cee_str *)(m->_);
}

static void S(noop)(void * v, enum cee_trace_action ta) {}
struct cee_block * cee_block_empty () {
  static struct S(header) singleton;
  singleton.cs.trace = S(noop);
  singleton.cs.resize_method = CEE_RESIZE_WITH_MALLOC;
  singleton.cs.mem_block_size = sizeof(struct S(header));
  singleton.capacity = 1;
  singleton._[0] = 0;
  return (struct cee_block *)&singleton._;
}

/*
 * if it's not NULL terminated, NULL should be returned
 */
char * cee_str_end(struct cee_str * str) {
  struct S(header) * b = FIND_HEADER(str);
  /* TODO: fixes this */
  return (char *)str + strlen((char *)str);
  /*
  int i = 0; 
  for (i = 0;i < b->used; i++)
    if (b->_[i] == '\0')
      return (b->_ + i);

  return NULL;
  */
}

/*
 * append any char (including '\0') to str;
 */
struct cee_str * cee_str_add(struct cee_str * str, char c) {
  struct S(header) * b = FIND_HEADER(str);
  uint32_t slen = strlen((char *)str);
  if( slen < b->capacity ){
    b->_[slen] = c;
    b->_[slen+1] = '\0';
    return (struct cee_str *)(b->_);
  }else{
    struct S(header) * b1 = S(resize)(b, _CEE_NEWSIZE(b->cs.mem_block_size, CEE_BLOCK));
    b1->capacity = b1->cs.mem_block_size - sizeof(struct S(header));
    b1->_[b->capacity] = c;
    b1->_[b->capacity+1] = '\0';
    return (struct cee_str *)(b1->_);
  }
}

struct cee_str * cee_str_catf(struct cee_str * str, const char * fmt, ...) {
  struct S(header) * b = FIND_HEADER(str);
  if (!fmt)
    return str;

  size_t slen = strlen((char *)str);

  va_list ap;
  va_start(ap, fmt);
  size_t s = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  s ++; /* including the null terminator */

  va_start(ap, fmt);
  if( slen + s < b->capacity ){
    vsnprintf(b->_ + slen, s, fmt, ap);
    va_end(ap);
    return str;
  }else{
    struct S(header) * b1 = S(resize)(b, _CEE_NEWSIZE(b->cs.mem_block_size, s));
    b1->capacity = b1->cs.mem_block_size - sizeof(struct S(header));
    vsnprintf(b1->_ + slen, s, fmt, ap);
    va_end(ap);
    return (struct cee_str *)(b1->_);
  }
}

struct cee_str * cee_str_ncat(struct cee_str * str, char * s, size_t slen){
  return NULL;
}

struct cee_str* cee_str_replace(struct cee_str *str, const char *fmt, ...) {
  struct S(header) * b = FIND_HEADER(str);
  if (!fmt)
    return str;

  va_list ap;
  va_start(ap, fmt);
  size_t s = vsnprintf(NULL, 0, fmt, ap);
  s ++; /* including the null terminator */

  va_start(ap, fmt);
  if( s < b->capacity ){
    vsnprintf(b->_, s, fmt, ap);
    return str;
  }else{
    struct S(header) *b1 = S(resize)(b, _CEE_NEWSIZE(b->cs.mem_block_size, s));
    b1->capacity = b1->cs.mem_block_size - sizeof(struct S(header));
    vsnprintf(b1->_, s, fmt, ap);
    return (struct cee_str *)(b1->_);
  }
}


void cee_str_rtrim(struct cee_str *s){
  int slen = strlen(s->_);
  for( int i = slen - 1; i > 0; i -- ){
    char c = s->_[i];
    if( c == ' ' || c == '\n' || c == '\r' || c == '\t' )
      s->_[i] = 0;
    else
      break;
  }
  return;
}

void cee_str_ltrim(struct cee_str *s){
  int start = 0;
  for( int i = 0; s->_[i]; i ++ ){
    char c = s->_[i];
    if( c == ' ' || c == '\n' || c == '\r' || c == '\t' )
      start = i;
    else
      break;
  }
  int new_len = strlen(s->_) - start;
  memmove(s->_, s->_ + start, new_len);
  s->_[new_len] = 0;
  return;
}


/*
 * replace len characters  at the offset in str with
 * s
 */ 
struct cee_str*
cee_str_replace_at_offset(struct cee_str *str, size_t offset, size_t len, char *new_substr){
  struct cee_state *state = cee_get_state(str);
  struct cee_str *rest = cee_str_mk(state, "%.*s",
				    strlen(str->_) - (offset + len),
				    str->_ + (offset + len));
  struct cee_str *head = cee_str_mk(state, "%.*s", offset, str->_);
  struct cee_str *ret = cee_str_replace(str, "%s%s%s", head, new_substr, rest->_);
  return ret;
}


/*
 * this is not super efficent, we can optimize this later to
 * reduce memory allocations and copies.
 */
struct cee_str*
cee_str_replace_all(struct cee_str *str, const char * old_substr, const char * new_substr){
  struct cee_str *first = str;
  size_t len = strlen(old_substr);
  char *pos = strstr(str->_, old_substr);
  struct cee_str *prev;
  while( pos != NULL ){
    prev = str;
    str = cee_str_replace_at_offset(str, pos - str->_, len, (char*)new_substr);
    if( prev != str && prev != first ) /* reallocated */
      cee_del(prev);
    pos = strstr(str->_ + len, old_substr);
  }
  return str;
}

#undef _CEE_NEWSIZE
