#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_boxed_##f
#else
#define  S(f)  _##f
#include "cee.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif
#include "cee-header.h"

struct S(header) {
  enum cee_boxed_primitive_type type;
  struct cee_sect cs;
  union cee_boxed_primitive_value _[1];
};
    
#include "cee-resize.h"

static void S(trace) (void * v, enum cee_trace_action ta) {
  struct S(header) * m = FIND_HEADER(v);
  switch(ta) {
    case CEE_TRACE_DEL_FOLLOW:
    case CEE_TRACE_DEL_NO_FOLLOW:
      S(de_chain)(m);
      free(m);
      break;
    case CEE_TRACE_MARK:
      m->cs.gc_mark = ta - CEE_TRACE_MARK;
      break;
  }
}

static int S(cmp) (void * v1, void * v2) {
  struct S(header) * h1 = FIND_HEADER(v1);
  struct S(header) * h2 = FIND_HEADER(v2);
  if (h1->cs.trace == h2->cs.trace)
    cee_segfault();
  else
    cee_segfault();
}


static struct S(header) * S(mk_header)(struct cee_state * s, enum cee_boxed_primitive_type t) {
  size_t mem_block_size = sizeof(struct S(header));
  struct S(header) * b = malloc(mem_block_size);
  ZERO_CEE_SECT(&b->cs);
  S(chain)(b, s);
  
  b->cs.trace = S(trace);
  b->cs.resize_method = CEE_RESIZE_WITH_IDENTITY;
  b->cs.mem_block_size = mem_block_size;
  b->cs.cmp = NULL;
  b->cs.n_product = 0;
  
  b->type = t;
  b->_[0].u64 = 0;
  return b;
}

static int S(cmp_double)(double v1, double v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_double (struct cee_state * s, double d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_f64);
  b->cs.cmp = (void *)S(cmp_double);
  b->_[0].f64 = d;
  return (struct cee_boxed *)b->_;
}

static int S(cmp_float)(float v1, float v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_float (struct cee_state * s, float d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_f32);
  b->cs.cmp = (void *)S(cmp_float);
  b->_[0].f32 = d;
  return (struct cee_boxed *)b->_;
}

static int S(cmp_u64)(uint64_t v1, uint64_t v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_u64 (struct cee_state * s, uint64_t d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_u64);
  b->_[0].u64 = d;
  return (struct cee_boxed *)b->_;
}

static int S(cmp_u32)(uint32_t v1, uint32_t v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_u32 (struct cee_state * s, uint32_t d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_u32);
  b->cs.cmp = (void *)S(cmp_u32);
  b->_[0].u32 = d;
  return (struct cee_boxed *)b->_;
}


static int S(cmp_u16)(uint16_t v1, uint16_t v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_u16 (struct cee_state * s, uint16_t d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_u16);
  b->cs.cmp = (void *) S(cmp_u16);
  b->_[0].u16 = d;
  return (struct cee_boxed *)b->_;
}


static int S(cmp_u8)(uint8_t v1, uint8_t v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_u8 (struct cee_state * s, uint8_t d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_u8);
  b->cs.cmp = (void *)S(cmp_u8);
  b->_[0].u8 = d;
  return (struct cee_boxed *)b->_;
}


static int S(cmp_i64)(int64_t v1, int64_t v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_i64 (struct cee_state *s, int64_t d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_i64);
  b->cs.cmp = (void *)S(cmp_i64);
  b->_[0].i64 = d;
  return (struct cee_boxed *)b->_;
}

static int S(cmp_i32)(int32_t v1, int32_t v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_i32 (struct cee_state * s, int32_t d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_i32);
  b->cs.cmp = (void *)S(cmp_i32);
  b->_[0].i32 = d;
  return (struct cee_boxed *)b->_;
}

static int S(cmp_i16)(int16_t v1, int16_t v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_i16 (struct cee_state * s, int16_t d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_i16);
  b->cs.cmp = (void *)S(cmp_i16);
  b->_[0].i16 = d;
  return (struct cee_boxed *)b->_;
}

static int S(cmp_i8)(int8_t v1, int8_t v2) {
  if (v1 == v2)
    return 0;
  else if (v1 > v2)
    return 1;
  else 
    return -1;
}

struct cee_boxed * cee_boxed_from_i8 (struct cee_state *s, int8_t d) {
  size_t mem_block_size = sizeof(struct cee_boxed);
  struct S(header) * b = S(mk_header)(s, cee_primitive_i8);
  b->cs.cmp = (void *)S(cmp_i8);
  b->_[0].i8 = d;
  return (struct cee_boxed *)b->_;
}

size_t cee_boxed_snprint (char * buf, size_t size, struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  int s;
  switch(h->type)
  {
    case cee_primitive_f64:
      s = snprintf(buf, size, "%lg", h->_[0].f64);
      break;
    case cee_primitive_f32:
      s = snprintf(buf, size, "%g", h->_[0].f32);
      break;
    case cee_primitive_i64:
      s = snprintf(buf, size, "%"PRId64, h->_[0].i64);
      break;
    case cee_primitive_u32:
    case cee_primitive_u16:
    case cee_primitive_u8:
      s = snprintf(buf, size, "%"PRIu32, h->_[0].u32);
      break;
    case cee_primitive_u64:
      s = snprintf(buf, size, "%"PRIu64, h->_[0].u64);
      break;
    case cee_primitive_i32:
    case cee_primitive_i16:
    case cee_primitive_i8:
      s = snprintf(buf, size, "%"PRId8, h->_[0].i8);
      break;
    default:
      cee_segfault();
      break;
  }
  if (s > 0)
    return (size_t)s;
  else
    cee_segfault();
}

double cee_boxed_to_double (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_f64)
    return h->_[0].f64;
  else
    cee_segfault();
}

float cee_boxed_to_float (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_f32)
    return h->_[0].f32;
  else
    cee_segfault();
}

uint64_t cee_boxed_to_u64 (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_u64)
    return h->_[0].u64;
  else
    cee_segfault();
}

uint32_t cee_boxed_to_u32 (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_u32)
    return h->_[0].u32;
  else
    cee_segfault();
}

uint16_t cee_boxed_to_u16 (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_u16)
    return h->_[0].u16;
  else
    cee_segfault();
}

uint8_t cee_boxed_to_u8 (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_u8)
    return h->_[0].u8;
  else
    cee_segfault();
}


int64_t cee_boxed_to_i64 (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_i64)
    return h->_[0].i64;
  else
    cee_segfault();
}

int32_t cee_boxed_to_i32 (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_i32)
    return h->_[0].i32;
  else
    cee_segfault();
}

int16_t cee_boxed_to_i16 (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_i16)
    return h->_[0].i16;
  else
    cee_segfault();
}

int8_t cee_boxed_to_i8 (struct cee_boxed * x) {
  struct S(header) * h = FIND_HEADER(x);
  if (h->type == cee_primitive_i8)
    return h->_[0].i8;
  else
    cee_segfault();
}
