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
 * new_substr
 */ 
char*
str_replace_at_offset(const char *str, size_t offset, size_t len, char *new_substr){
  size_t i, i_src = 0, i_dest = 0, new_substr_len = strlen(new_substr);
  size_t orig_len = strlen(str);
  
  char *new_str = malloc(orig_len - len + new_substr_len + 1);
  
  for( i = 0; i < offset; i++ )
    new_str[i] = str[i];

  for( i_dest = 0; i_dest < new_substr_len; i_dest ++ )
    new_str[i+i_dest] = new_substr[i_dest];

  i_dest += i;

  i += len; /* skip len */
  while( i < orig_len){
    new_str[i_dest] = str[i];
    i++;
    i_dest ++;
  }

  new_str[i_dest] = 0;
  return new_str;
}


/*
 * this is not super efficent, we can optimize this later to
 * reduce memory allocations and copies.
 */
char*
str_replace_all(const char *str, const char *old_substr, const char *new_substr){
  size_t old_len = strlen(old_substr), new_len = strlen(new_substr), orig_len = strlen(str);
  double ratio = new_len/old_len + 1;
  char  *new_str = malloc(orig_len * ratio);

  int i_src = 0, i_dest = 0;
  while( i_src < orig_len ){
    if( strncmp(str+i_src, old_substr, old_len) == 0 ){
      for( int n = 0; n < new_len; n++ )
        new_str[i_dest+n] = new_substr[n];
      i_dest += new_len;
      i_src += old_len;
    }else{
      new_str[i_dest] = str[i_src];
      i_dest ++;
      i_src ++;
    }
  }
  new_str[i_dest] = 0;
  return new_str;
}



char* str_replace_all_ext(const char *str, size_t str_len, size_t *out_size, int n_pairs, ...){
  struct pair {
    char *old_needle, *new_needle;
    size_t old_len, new_len;
  };
  int min_old_len = 1000, max_new_len = 0, max_len, min_len;
  char *old_needle, *new_needle;

  struct pair *pairs, *used_pair = NULL;
  if( n_pairs <= 16 )
     pairs = alloca(sizeof(struct pair) * n_pairs);
  else
     pairs = malloc(sizeof(struct pair) * n_pairs);

  va_list ap;
  va_start(ap, n_pairs);
  for( int i = 0; i < n_pairs; i++ ){
    pairs[i].old_needle = va_arg(ap, char*);
    pairs[i].new_needle = va_arg(ap, char*);
    
    pairs[i].old_len = strlen(pairs[i].old_needle);
    pairs[i].new_len = strlen(pairs[i].new_needle);
    
    if( pairs[i].old_len < min_old_len )
      min_old_len = pairs[i].old_len;
    
    if( pairs[i].new_len > max_new_len )
      max_new_len = pairs[i].new_len;
  }
  va_end(ap);

  max_len = max_new_len > min_old_len ? max_new_len : min_old_len;
  min_len = max_new_len > min_old_len ? min_old_len : max_new_len;

  size_t new_strlen = (str_len/min_len) * max_len; /* make it larger to be safer */
  char *new_str = malloc(new_strlen + 1);

  size_t i_src = 0, i_dest = 0;
  while( i_src < str_len ){
    used_pair = NULL;
    for( int x = 0; x < n_pairs; x++ ){
      if( i_src + pairs[x].old_len < str_len
          && strncmp(str+i_src, pairs[x].old_needle, pairs[x].old_len) == 0 ){
        used_pair = pairs+x;
        break;
      }
    }
    if( used_pair ){
      for( int n = 0; n < used_pair->new_len; n++ )
        new_str[i_dest+n] = used_pair->new_needle[n];
      i_dest += used_pair->new_len;
      i_src += used_pair->old_len;
    }else{
      new_str[i_dest] = str[i_src];
      i_dest ++;
      i_src ++;
    }
  }
  new_str[i_dest] = 0;
  if( out_size ) 
    *out_size = i_dest;
  if( n_pairs > 16 )
    free(pairs);
  return new_str;
}

int str_ends_with(const char *str, const char *suffix){
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  return str_len > suffix_len && !strcmp(str + (str_len - suffix_len), suffix);
}

#undef _CEE_NEWSIZE
