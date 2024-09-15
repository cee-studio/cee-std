#ifndef CEE_JSON_ONE
#define CEE_JSON_ONE
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include "cee.h"
 
#ifndef CEE_JSON_H
#define CEE_JSON_H
#ifndef CEE_JSON_AMALGAMATION
#include "cee.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h> /* ssize_t */
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_JSON_DEPTH 500

enum cee_json_type {
  CEE_JSON_UNDEFINED,    /**<  Undefined value */
  CEE_JSON_NULL,         /**<  null value */
  CEE_JSON_BOOLEAN,      /**<  boolean value */
  CEE_JSON_DOUBLE,       /**<  double value */
  CEE_JSON_I64,          /**<  64-bit signed int */
  CEE_JSON_U64,          /**<  64-bit unsigned int */
  CEE_JSON_STRING,       /**<  string value */
  CEE_JSON_BLOB,         /**<  blob value, it should be printed as base64 */
  CEE_JSON_OBJECT,       /**<  object value  */
  CEE_JSON_ARRAY,        /**<  array value */
  CEE_JSON_STRN
};

struct cee_json {
  enum cee_json_type t;
  union {
    struct cee_singleton * null;
    struct cee_singleton * undefined;
    struct cee_singleton * boolean;
    struct cee_boxed     * boxed;
    struct cee_str       * string;
    struct cee_block     * blob;
    struct cee_list      * array;
    struct cee_map       * object;
  } value;
  char *buf;
  size_t buf_size;
};

enum cee_json_fmt {
  CEE_JSON_FMT_COMPACT = 0,
  CEE_JSON_FMT_PRETTY  = 1
};

/*
 *
 */
extern struct cee_json* cee_json_select (struct cee_json *, char *selector, ...);

extern int cee_json_select_as_int (struct cee_json *, int *, char *selector);

extern bool cee_json_save_fileno(struct cee_state *, struct cee_json *, int fd, int how);
extern bool cee_json_save (struct cee_state *, struct cee_json *, FILE *, int how);
extern struct cee_json * cee_json_load_from_fileno(struct cee_state *,
                                                   int fd, bool force_eof, 
                                                   int * error_at_line);
extern struct cee_json * cee_json_load_from_FILE(struct cee_state *,
                                                 FILE *, bool force_eof, 
                                                 int * error_at_line);
struct cee_json * cee_json_load_from_file_path(struct cee_state *st,
                                               char *file_path, bool force_eof, 
                                               int *error_at_line);
extern struct cee_json *cee_json_load_from_buffer(char *buf, size_t buf_size);
extern int cee_json_cmp (struct cee_json *, struct cee_json *);

extern bool cee_json_merge (struct cee_json *dest, struct cee_json *src);
extern bool cee_json_merge_or_return (struct cee_json **dest_p, struct cee_json *src);

extern struct cee_json * cee_list_to_json (struct cee_list *v);
extern struct cee_json * cee_map_to_json (struct cee_map *v);

extern struct cee_list * cee_json_to_array (struct cee_json *);
extern struct cee_map * cee_json_to_object (struct cee_json *);
extern struct cee_boxed * cee_json_to_boxed (struct cee_json *);
extern struct cee_str* cee_json_to_str (struct cee_json *);
extern bool cee_json_to_strn(struct cee_json *p, char **start_p, size_t *size_p);
extern struct cee_block* cee_json_to_blob (struct cee_json *);
/*
 * convert object/str to [object/str]
 */
extern struct cee_json* cee_json_listify(struct cee_json *);

extern int cee_json_to_doublex (struct cee_json *, double *);
extern int cee_json_to_intx (struct cee_json*, int *);
extern int cee_json_to_i64x (struct cee_json*, int64_t *);
extern int cee_json_to_u64x (struct cee_json*, uint64_t *);
extern int cee_json_to_boolx(struct cee_json*, bool *);

extern struct cee_json * cee_json_true ();
extern struct cee_json * cee_json_false ();
extern struct cee_json * cee_json_bool (bool b);
extern struct cee_json * cee_json_undefined ();
extern struct cee_json * cee_json_null ();
extern struct cee_json * cee_json_object_mk (struct cee_state *);
extern struct cee_json * cee_json_object_kv (struct cee_state *, char *key, struct cee_json *value);

extern struct cee_json * cee_json_double_mk (struct cee_state *, double d);
extern struct cee_json * cee_json_i64_mk(struct cee_state *, int64_t);
extern struct cee_json * cee_json_u64_mk(struct cee_state *, uint64_t);
extern struct cee_json * cee_json_str_mk(struct cee_state *, struct cee_str * s);
extern struct cee_json * cee_json_str_mkf(struct cee_state *, const char *fmt, ...);
extern struct cee_json * cee_json_blob_mk(struct cee_state *, const void *src, size_t bytes);
extern struct cee_json * cee_json_array_mk(struct cee_state *, int s);
extern struct cee_json * cee_json_strn_mk(struct cee_state *, char *start, size_t size);

/*
 * return 1 if cee_json is an empty object or is an empty array
 * return 0 otherwise
 */
extern int cee_json_empty(struct cee_json *);

extern void cee_json_object_set (struct cee_json *, char *, struct cee_json *);
/*
 * if the key has an array value, append to the end
 * otherwise, convert the value to an array of this value, and append to the end
 */
extern void cee_json_object_append (struct cee_json *, char *, struct cee_json *);
extern void cee_json_object_set_bool (struct cee_json *, char *, bool);
extern void cee_json_object_set_str (struct cee_json *, char *, char *);
extern void cee_json_object_set_strf (struct cee_json *, char *, const char *fmt, ...);
extern void cee_json_object_set_double (struct cee_json *, char *, double);
extern void cee_json_object_set_i64 (struct cee_json *, char *, int64_t);
extern void cee_json_object_set_u64 (struct cee_json *, char *, uint64_t);
extern bool cee_json_object_rename (struct cee_json *, char *old_key, char *new_key);
extern bool cee_json_object_convert_to_i64 (struct cee_json*, char *key);

extern void cee_json_set_error(struct cee_json **o, const char *fmt, ...);

extern struct cee_json* cee_json_object_get(struct cee_json *, char *key);
/* remove a key from a json object */
extern void cee_json_object_remove (struct cee_json *, char *);

extern int cee_json_object_iterate (struct cee_json *, void *ctx, 
                                    int (*f)(void *ctx, struct cee_str *key, struct cee_json *val));

extern void cee_json_array_insert(struct cee_json * j, int idx, struct cee_json *v);
extern void cee_json_array_append (struct cee_json *, struct cee_json *);
extern void cee_json_array_append_bool (struct cee_json *, bool);
extern void cee_json_array_append_str (struct cee_json *, char *);
extern void cee_json_array_append_strf (struct cee_json *, const char *fmt, ...);
extern void cee_json_array_append_double (struct cee_json *, double);
extern void cee_json_array_append_i64 (struct cee_json *, int64_t);
extern void cee_json_array_append_u64 (struct cee_json *, uint64_t);
extern void cee_json_array_concat (struct cee_json *, struct cee_json *);

/* remove an element from a json array */
extern void cee_json_array_remove (struct cee_json *, int index);

extern size_t cee_json_array_length (struct cee_json *);
extern struct cee_json* cee_json_array_get(struct cee_json *, int);
extern int cee_json_array_iterate (struct cee_json *, void *ctx,
                                   int (*f)(void *ctx, struct cee_json *val, int index));

extern ssize_t cee_json_snprint (struct cee_state *, char *buf,
				 size_t size, struct cee_json *json,
				 enum cee_json_fmt);

/*
 * buf_p points to a memory block that is tracked by cee_state
 * if buf_len is not NULL, it contains the size of the memory block
 */
extern ssize_t cee_json_asprint (struct cee_state *, char **buf_p, size_t *buf_size_p,
                                 struct cee_json *json, enum cee_json_fmt);

void cee_json_dprintf(int fd, struct cee_json *json, const char *fmt, ...);

extern int cee_json_parsex(struct cee_state *st, char *buf, uintptr_t len, struct cee_json **out, 
			   bool force_eof, int *error_at_line);

/*
 * return the value if this json has this key in anyone of its children
 * transitively and recursively
 */
extern struct cee_json* cee_json_has(struct cee_json *, char *key);

extern int cee_json_merge_all(char **json_files, int json_file_count, char *merged);

#ifdef __cplusplus
}
#endif

#endif /* CEE_JSON_H */
 
#ifndef CEE_JSON_TOKENIZER_H
#define CEE_JSON_TOKENIZER_H
enum token {
  tock_eof = 255,
  tock_err,
  tock_str,
  tock_number,
  tock_true,
  tock_false,
  tock_null
};

struct tokenizer {
  int line;
  char * buf;
  char * buf_end;
  struct cee_str * str;
  enum number_type {
     NUMBER_IS_DOUBLE,
     NUMBER_IS_I64,
     NUMBER_IS_U64
  } type;
  union {
    double real;
    int64_t i64;
    uint64_t u64;
  } number;
};

extern enum token cee_json_next_token(struct cee_state *, struct tokenizer * t);
#endif /* CEE_JSON_TOKENIZER_H */
 

struct cee_json * cee_json_true () {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init(b, (uintptr_t)CEE_JSON_BOOLEAN, 1);
}

struct cee_json * cee_json_false () {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init(b, (uintptr_t)CEE_JSON_BOOLEAN, 0);
}

struct cee_json * cee_json_bool (bool b) {
  return b ? cee_json_true() : cee_json_false();
}

struct cee_json * cee_json_undefined () {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init(b, (uintptr_t)CEE_JSON_UNDEFINED, 0);
}

struct cee_json * cee_json_null () {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init(b, (uintptr_t)CEE_JSON_NULL, 0);
}

struct cee_json * cee_list_to_json(struct cee_list *v) {
  struct cee_state *st = cee_get_state(v);
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_ARRAY, v);
}

struct cee_json * cee_map_to_json(struct cee_map *v) {
  struct cee_state *st = cee_get_state(v);
  /* TODO: check v use strcmp as the comparison function */
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_OBJECT, v);
}

struct cee_json* cee_json_listify(struct cee_json *j)
{
  switch (j->t) {
  case CEE_JSON_ARRAY:
    return j;
  case CEE_JSON_OBJECT: {
    struct cee_state *state = cee_get_state(cee_json_to_str(j));
    struct cee_json *list = cee_json_array_mk(state, 1);
    cee_json_array_append(list, j);
    return list;
  }
  case CEE_JSON_STRING: {
    struct cee_state *state = cee_get_state(cee_json_to_str(j));
    struct cee_json *list = cee_json_array_mk(state, 1);
    cee_json_array_append(list, j);
    return list;
  }
  default:
    cee_segfault();
  }
}

struct json_has_ctx {
  char *key;
  struct cee_json* found;
};

static int find_in_elem(void *ctx, void *val, int idx){
  struct json_has_ctx *has_ctx = ctx;
  has_ctx->found = cee_json_has(val, has_ctx->key);
  return (NULL != has_ctx->found); /* stop if found is not null */
}

static int match_key(void *ctx, void *key, void *val){
  struct json_has_ctx *has_ctx = ctx;
  if (strcmp(key, has_ctx->key) == 0) {
    has_ctx->found = val;
    return 1; /* stop */
  }
  has_ctx->found = cee_json_has(val, has_ctx->key);
  return (NULL != has_ctx->found); /* stop if found is not null */
}

struct cee_json* cee_json_has(struct cee_json *j, char *key) {
  struct json_has_ctx ctx = {0};
  ctx.key = key;
  int ret;

  switch (j->t) {
  case CEE_JSON_ARRAY: {
    cee_list_iterate(j->value.array, &ctx, find_in_elem);
    return ctx.found;
  }
  case CEE_JSON_OBJECT: {
    cee_map_iterate(j->value.object, &ctx, match_key);
    return ctx.found;
  }
  default:
    return NULL;
  }
}

struct cee_map * cee_json_to_object (struct cee_json *p) {
  return (p->t == CEE_JSON_OBJECT) ? p->value.object : NULL;
}

struct cee_list * cee_json_to_array (struct cee_json *p) {
  return (p->t == CEE_JSON_ARRAY) ? p->value.array : NULL;
}

struct cee_str * cee_json_to_str (struct cee_json *p) {
  return (p->t == CEE_JSON_STRING) ? p->value.string : NULL;
}

bool cee_json_to_strn (struct cee_json *p, char **start_p, size_t *size_p) {
  if( p->t == CEE_JSON_STRN ){
    *start_p = p->buf;
    *size_p = p->buf_size;
    return true;
  }else{
    return false;
  }
}

struct cee_boxed * cee_json_to_boxed (struct cee_json *p) {
  if (p->t == CEE_JSON_U64 || p->t == CEE_JSON_I64 || p->t == CEE_JSON_DOUBLE)
    return p->value.boxed;
  else
    return NULL;
}

struct cee_block* cee_json_to_blob (struct cee_json *p) {
  if (p->t == CEE_JSON_BLOB)
    return p->value.blob;
  else
    return NULL;
}



int cee_json_empty(struct cee_json *p) {
  switch(p->t) {
  case CEE_JSON_OBJECT:
    return cee_map_size(p->value.object) == 0;
  case CEE_JSON_ARRAY:
    return cee_list_size(p->value.array) == 0;
  default:
    return 0;
  }
}


int cee_json_to_doublex (struct cee_json *p, double *r) {
  if (p->t == CEE_JSON_DOUBLE) {
    *r = cee_boxed_to_double(p->value.boxed);
    return 0;
  }
  else
    return EINVAL;
}

int cee_json_to_intx (struct cee_json *p, int *r) {
  if (p->t == CEE_JSON_I64) {
    *r = (int)cee_boxed_to_i64(p->value.boxed);
    return 0;
  }
  else
    return EINVAL;
}

int cee_json_to_i64x (struct cee_json *p, int64_t *r) {
  if (p->t == CEE_JSON_I64) {
    *r = cee_boxed_to_i64(p->value.boxed);
    return 0;
  }
  else
    return EINVAL;
}

int cee_json_to_u64x (struct cee_json *p, uint64_t *r) {
  if (p->t == CEE_JSON_U64) {
    *r = cee_boxed_to_u64(p->value.boxed);
    return 0;
  }
  else
    return EINVAL;
}

int cee_json_to_boolx(struct cee_json *p, bool *r) {
  if (p == cee_json_true()) {
    *r = true;
    return 0;
  }
  else if (p == cee_json_false()) {
    *r = false;
    return 0;
  }
  else
    return EINVAL;
}

static void* merge_json_value (void *ctx, void *oldv, void *newv)
{
  struct cee_json *oldj = oldv, *newj = newv;
  if (cee_json_merge(oldj, newj))
    return oldj;
  else
    return newj;
}

/*
 * merge two objects recursively by combining 
 * their key/value pairs:
 * 
 * if the src value and dest value are different types,
 * the src value overwrite the dest value.
 *
 * if both src value and dest value are objects, merge
 * them recursively.
 *
 */
bool cee_json_merge (struct cee_json *dest, struct cee_json *src) {
  if (NULL == dest || NULL == src) return false;
  if (dest->t == src->t) {
    if (dest->t == CEE_JSON_OBJECT) {
      struct cee_map *dest_map = cee_json_to_object(dest);
      struct cee_map *src_map = cee_json_to_object(src);
      cee_map_merge(dest_map, src_map, NULL, merge_json_value);
      return true;
    }
    else if (dest->t == CEE_JSON_ARRAY) {
      struct cee_list *dest_list = cee_json_to_array(dest);
      struct cee_list *src_list = cee_json_to_array(src);
      cee_list_merge(dest_list, src_list);
      return true;
    }
    return false;
  }
  else
    return false;
}

bool cee_json_merge_or_return (struct cee_json **dest_p, struct cee_json *src) {
  if (NULL == src || NULL == dest_p) return false;
  if (NULL == *dest_p) {
    *dest_p = src;
    return true;
  }
  return cee_json_merge (*dest_p, src);
}


struct cee_json * cee_json_double_mk (struct cee_state *st, double d) {
  struct cee_boxed * p = cee_boxed_from_double (st, d);
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_DOUBLE, p);
}

struct cee_json * cee_json_i64_mk (struct cee_state *st, int64_t d) {
  struct cee_boxed * p = cee_boxed_from_i64 (st, d);
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_I64, p);
}

struct cee_json * cee_json_u64_mk (struct cee_state *st, uint64_t d) {
  struct cee_boxed * p = cee_boxed_from_u64 (st, d);
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_U64, p);
}

struct cee_json * cee_json_str_mk(struct cee_state *st, struct cee_str *s) {
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_STRING, s);
}

struct cee_json * cee_json_str_mkf(struct cee_state *st, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  struct cee_str *s = cee_str_mkv(st, fmt, ap);
  va_end(ap);
  return cee_json_str_mk(st, s);
}

struct cee_json * cee_json_array_mk(struct cee_state *st, int s) {
  struct cee_list * v = cee_list_mk (st, s);
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_ARRAY, v);
}

struct cee_json * cee_json_object_mk(struct cee_state *st) {
  struct cee_map * m = cee_map_mk (st, (cee_cmp_fun)strcmp);
  struct cee_tagged * t = cee_tagged_mk (st, CEE_JSON_OBJECT, m);
  return (struct cee_json *)t;
}

struct cee_json * cee_json_object_kv(struct cee_state *st, char *key, struct cee_json *value) {
  struct cee_json *j = cee_json_object_mk(st);
  struct cee_map * m = cee_json_to_object(j);
  cee_json_object_set (j, key, value);
  return j;
}

struct cee_json* cee_json_blob_mk(struct cee_state *st, const void *src, size_t bytes) {
  struct cee_block *m = cee_block_mk_nonzero(st, bytes);
  memcpy(m->_, src, bytes);
  struct cee_tagged *t = cee_tagged_mk (st, CEE_JSON_BLOB, m);
  return (struct cee_json*)t;
}

struct cee_json* cee_json_strn_mk(struct cee_state *st, char *start, size_t bytes){
  struct cee_tagged *t = cee_tagged_mk (st, CEE_JSON_STRN, NULL);
  t->buf = start;
  t->buf_size = bytes;
  return (struct cee_json*)t;
}

bool cee_json_object_rename(struct cee_json *j, char *old_key, char *new_key) {
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    return false;
  struct cee_state *st = cee_get_state(o);
  struct cee_str *old_str = cee_str_mk(st, "%s", old_key);
  struct cee_str *new_str = cee_str_mk(st, "%s", new_key);
  bool t = cee_map_rename(o, old_str, new_str);
  cee_del(old_str);
  return t;
}

static int str2i64(char *s, int64_t *ip) {
  int64_t i = 0;
  char c ;
  int scanned = sscanf(s, "%" SCNd64 "%c", &i, &c);
  if (scanned == 1) {
    if (ip) *ip = i;
    return 0;
  }
  if (scanned > 1) {
    return 1;
  }
  return 1;
}

bool cee_json_object_convert_to_i64(struct cee_json *j, char *key) {
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    return false;

  struct cee_json *old_val = cee_json_object_get(j, key);
  int64_t i64 = 0;
  if (!cee_json_to_i64x(old_val, &i64))
    return true;

  if (j->t == CEE_JSON_STRING && !str2i64(j->value.string->_, &i64)) {
    cee_json_object_set_i64(j, key, i64);
    cee_del(old_val);
    return true;
  }
  return false;
}

struct cee_json* cee_json_object_get(struct cee_json *j, char *key)
{
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();
  return cee_map_find(o, key);
}

void cee_json_object_remove(struct cee_json *j, char *key)
{
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();
  cee_map_remove(o, key);
}

void cee_json_set_error(struct cee_json **x, const char *fmt, ...) {
  if (x == NULL || *x == NULL) return;
  struct cee_json *o = *x;
  if (o->t != CEE_JSON_OBJECT)
    cee_segfault();
  if (cee_json_select(o, ".error"))
    /* don't overwrite the previous error */
    return;

  struct cee_state *state = cee_get_state(o->value.object);
  va_list ap;
  va_start(ap, fmt);
  struct cee_str *str = cee_str_mkv(state, fmt, ap);
  va_end(ap);
  cee_json_object_set(o, "error", cee_json_str_mk(state, str));
}


static int is_json(struct cee_json *v){
  if( v->t <= CEE_JSON_STRN )
    return 1;
  return 0;
}

void cee_json_object_set(struct cee_json *j, char *key, struct cee_json *v) {
  if (NULL == j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  if( !is_json(v) )
    cee_segfault();
  cee_map_add(o, cee_str_mk(st, "%s", key), v);
}

void cee_json_object_append(struct cee_json *j, char *key, struct cee_json *v) {
  if (NULL == j || NULL == v) return;
  struct cee_json *old_value = cee_json_object_get(j, key);
  if (old_value) {
    if (cee_json_to_array(old_value)) {
      if (cee_json_to_array(v))
        cee_json_array_concat(old_value, v);
      else
        cee_json_array_append(old_value, v);
    }
    else {
      struct cee_json *llist = cee_json_listify(old_value);
      if (cee_json_to_array(v))
        cee_json_array_concat(llist, v);
      else
        cee_json_array_append(llist, v);
      cee_json_object_set(j, key, llist);
    }
  }
  else
    cee_json_object_set(j, key, v);
}

void cee_json_object_set_bool(struct cee_json *j, char *key, bool b) {
  if (NULL == j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_bool(b));
}

void cee_json_object_set_str (struct cee_json *j, char *key, char *str) {
  if (NULL == j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_str_mk(st, cee_str_mk(st, "%s", str)));
}

void cee_json_object_set_strf (struct cee_json *j, char *key, const char *fmt, ...) {
  if (NULL == j) return;
  va_list ap;
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();

  struct cee_state *st = cee_get_state(o);
  va_start(ap, fmt);
  struct cee_str *v = cee_str_mkv(st, fmt, ap);
  va_end(ap);

  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_str_mk(st, v));
}

void cee_json_object_set_double (struct cee_json *j, char *key, double real) {
  if (NULL == j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_double_mk(st, real));
}

void cee_json_object_set_i64 (struct cee_json *j, char *key, int64_t real) {
  if (NULL == j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_i64_mk(st, real));
}

void cee_json_object_set_u64 (struct cee_json *j, char *key, uint64_t real) {
  if (NULL == j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_u64_mk(st, real));
}

int cee_json_object_iterate (struct cee_json *j, void *ctx,
                              int (*f)(void *ctx, struct cee_str *key, struct cee_json *value))
{
  struct cee_map *o = cee_json_to_object(j);
  if (NULL == o)
    cee_segfault();
  typedef int (*fnt)(void *, void*, void*);
  return cee_map_iterate(o, ctx, (fnt)f);
};

void cee_json_array_insert(struct cee_json * j, int idx, struct cee_json *v) {
  struct cee_list *o = cee_json_to_array(j);
  if (NULL == o)
    cee_segfault();
  cee_list_insert(o, idx, v);
  if (o != j->value.array) {
    /*  free j->value.array */
    j->value.array = o;
  }
}

void cee_json_array_append (struct cee_json * j, struct cee_json *v) {
  struct cee_list *o = cee_json_to_array(j);
  if (NULL == o)
    cee_segfault();
  cee_list_append(o, v);
  if (o != j->value.array) {
    /*  free j->value.array */
    j->value.array = o;
  }
}

void cee_json_array_append_bool (struct cee_json * j, bool b) {
  struct cee_list * o = cee_json_to_array(j);
  if (NULL == o)
    cee_segfault();
  cee_list_append(o, cee_json_bool(b));
  if (o != j->value.array) {
    /*  free j->value.array */
    j->value.array = o;
  }
}

void cee_json_array_append_str (struct cee_json * j, char * x) {
  struct cee_list *o = cee_json_to_array(j);
  if (NULL == o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_list_append(o, cee_json_str_mk(st, cee_str_mk(st, "%s", x)));
  if (o != j->value.array) {
    /*  free j->value.array */
    j->value.array = o;
  }
}

void cee_json_array_append_strf (struct cee_json *j, const char *fmt, ...) {
  va_list ap;
  struct cee_list *o = cee_json_to_array(j);
  if (NULL == o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  va_start(ap, fmt);
  struct cee_str *v = cee_str_mkv(st, fmt, ap);
  va_end(ap);

  cee_list_append(o, cee_json_str_mk(st, v));
  if (o != j->value.array) {
    /*  free j->value.array */
    j->value.array = o;
  }
}

size_t cee_json_array_length(struct cee_json *j) {
  struct cee_list *o = cee_json_to_array(j);
  if (NULL == o)
    cee_segfault();
  return cee_list_size(o);
}

struct cee_json* cee_json_array_get (struct cee_json *j, int i) {
  struct cee_list *o = cee_json_to_array(j);
  if (NULL == o)
    cee_segfault();
  if (0 <= i && i < cee_list_size(o))
    return o->a[i];
  else
    return NULL;
}

void cee_json_array_remove(struct cee_json *j, int i) {
  struct cee_list *o = cee_json_to_array(j);
  if (NULL == o)
    cee_segfault();
  if (0 <= i && i < cee_list_size(o))
    cee_list_remove(o, i);
}

int cee_json_array_iterate (struct cee_json *j, void *ctx,
                            int (*f)(void *ctx, struct cee_json *value, int index))
{
  struct cee_list *o = cee_json_to_array(j);
  if (NULL == o)
    cee_segfault();
  typedef int (*fnt)(void *, void*, int);
  return cee_list_iterate(o, ctx, (fnt)f);
}

void cee_json_array_concat (struct cee_json *dest, struct cee_json *src)
{
  struct cee_list *d = cee_json_to_array(dest);
  struct cee_list *s = cee_json_to_array(src);
  if (!d || !s)
    cee_segfault();

  int i;
  for (i = 0; i < cee_list_size(s); i++)
    cee_list_append(d, s->a[i]);
}

/*
 * this function assume the file pointer points to the begin of a file
 */
struct cee_json * cee_json_load_from_fileno(struct cee_state * st,
                                           int fd, bool force_eof,
                                           int * error_at_line) {
  struct stat buf;
  fstat(fd, &buf);
  off_t size = buf.st_size;
  char * b = malloc(size);
  read(fd, b, size);

  int line = 0;
  struct cee_json * j = NULL;
  if( cee_json_parsex(st, b, size, &j, true, &line) ){
    /*  report error */
    fprintf(stderr, "failed to parse at %d\n", line);
    *error_at_line = line;
    j = NULL;
  }
  free(b);
  return j;
}
/*
 * this function assume the file pointer points to the begin of a file
 */
struct cee_json * cee_json_load_from_FILE(struct cee_state * st,
                                          FILE * f, bool force_eof,
                                          int * error_at_line){
  return cee_json_load_from_fileno(st, fileno(f), force_eof, error_at_line);
}

/*
 * 
 */
struct cee_json* cee_json_load_from_file_path(struct cee_state *st,
                                              char *file_path, bool force_eof,
                                              int *error_at_line){
  FILE *fp = fopen(file_path, "r");
  if( fp == NULL ){
     fprintf(stderr, "failed to open %s", file_path);
     cee_segfault();
  }
  struct cee_json *ret = cee_json_load_from_FILE(st, fp, force_eof, error_at_line);
  fclose(fp);
  return ret;
}


struct cee_json *cee_json_load_from_buffer(char *buf, size_t buf_size){
  struct cee_state *st = cee_state_mk(512);
  struct cee_json *j = NULL;
  int line = 0;
  if( cee_json_parsex(st, buf, buf_size, &j, true, &line) ){
    fprintf(stderr, "failed to parse at %d\n", line);
    j = NULL;
  }
  return j;
}

bool cee_json_save_fileno(struct cee_state * st, struct cee_json * j, int fd, int how) {
  size_t s = cee_json_snprint (st, NULL, 0, j, how);
  char * p = malloc(s+1);
  cee_json_snprint (st, p, s+1, j, how);
  if( write(fd, p, s) != s ){
    fprintf(stderr, "%s", strerror(errno));
    free(p);
    return false;
  }
  free(p);
  return true;
}

bool cee_json_save(struct cee_state * st, struct cee_json * j, FILE *f, int how) {
  return cee_json_save_fileno(st, j, fileno(f), how);
}


/*
 * The following code is shamelessly stolen from the 
 * great work of antirez's stonky:
 * https://github.com/antirez/stonky/blob/7260d9de9c6ae60776125842643ada84cc7e5ec6/stonky.c#L309
 * the LICENSE of this code belongs to Salvatore Sanfilippo <antirez at gmail dot com>
 *
 * It is modified to work with cee_json with minimum changes
 *
 * You can select things like this:
 *
 * cee_json *width = cee_json_select(json,".features.screens[*].width",4);
 * cee_json *height = cee_json_select(json,".features.screens[4].*","height");
 * cee_json *price = cee_json_select(json,".features.screens[4].price_*",
 *                  price_type == EUR ? "eur" : "usd");
 *
 * You can use a ":<type>" specifier, usually at the end, in order to
 * check the type of the final JSON object selected. If the type will not
 * match, the function will return NULL. For instance the specifier:
 *
 *  ".foo.bar:s"
 *
 * Will not return NULL only if the root object has a foo field, that is
 * an object with a bat field, that contains a string. This is the full
 * list of selectors:
 *
 *  ".field", select the "field" of the current object.
 *  "[1234]", select the specified index of the current array.
 *  ":<type>", check if the currently selected type is of the specified type,
 *             where the type is a single letter that can be:
 *             "s" for string
 *             "n" for number
 *             "a" for array
 *             "o" for object
 *             "b" for boolean
 *             "!" for null
 *
 * Selectors can be combined, and the special "*" can be used in order to
 * fetch array indexes or field names from the arguments:
 *
 *      cee_json *myobj = cee_json_select(root,".properties[*].*", index, fieldname);
 */
struct cee_json* cee_json_select(struct cee_json *o, char *fmt, ...) {
  if (NULL == o) return NULL;

  enum next_selector_token {
    JSEL_INVALID = 0,
    JSEL_OBJ = 1, /* "." */
    JSEL_ARRAY = 2, /* "[" */
    JSEL_TYPECHECK = 3, /* ":" */
    JSEL_MAX_TOKEN = 256
  } next = JSEL_INVALID; /* Type of the next selector. */
  char token[JSEL_MAX_TOKEN+1]; /* Current token. */
  int tlen; /* Current length of the token. */
  va_list ap;

  va_start(ap,fmt);
  const char *p = fmt;
  tlen = 0;
  while(1) {
    /* Our four special chars (plus the end of the string) signal the
     * end of the previous token and the start of the next one. */
    if (tlen && (*p == '\0' || strchr(".[]:",*p))) {
      token[tlen] = '\0';
      if (next == JSEL_INVALID) {
        goto notfound;
      } else if (next == JSEL_ARRAY) {
        if (o->t != CEE_JSON_ARRAY) goto notfound;
        int idx = atoi(token); /* cee_json API index is int. */
        if ((o = cee_json_array_get(o,idx)) == NULL)
          goto notfound;
      } else if (next == JSEL_OBJ) {
        if (o->t != CEE_JSON_OBJECT) goto notfound;
        if ((o = cee_json_object_get(o,token)) == NULL)
          goto notfound;
      } else if (next == JSEL_TYPECHECK) {
        if (token[0] == 's' && o->t != CEE_JSON_STRING) goto notfound;
        if (token[0] == 'n' && o->t != CEE_JSON_I64) goto notfound;
        if (token[0] == 'o' && o->t != CEE_JSON_OBJECT) goto notfound;
        if (token[0] == 'a' && o->t != CEE_JSON_ARRAY) goto notfound;
        if (token[0] == 'b' && o->t != CEE_JSON_BOOLEAN) goto notfound;
        if (token[0] == '!' && o->t != CEE_JSON_NULL) goto notfound;
      }
    } else if (next != JSEL_INVALID) {
      /* Otherwise accumulate characters in the current token, note that
       * the above check for JSEL_NEXT_INVALID prevents us from
       * accumulating at the start of the fmt string if no token was
       * yet selected. */
      if (*p != '*') {
        token[tlen] = *p++;
        tlen++;
        if (tlen > JSEL_MAX_TOKEN) goto notfound;
        continue;
      } else {
        /* The "*" character is special: if we are in the context
         * of an array, we read an integer from the variable argument
         * list, then concatenate it to the current string.
         *
         * If the context is an object, we read a string pointer
         * from the variable argument string and concatenate the
         * string to the current token. */
        int len;
        char buf[64];
        char *s;
        if (next == JSEL_ARRAY) {
          int idx = va_arg(ap,int);
          len = snprintf(buf,sizeof(buf),"%d",idx);
          s = buf;
        } else if (next == JSEL_OBJ) {
          s = va_arg(ap,char*);
          len = strlen(s);
        } else {
          goto notfound;
        }
        /* Common path. */
        if (tlen+len > JSEL_MAX_TOKEN) goto notfound;
        memcpy(token+tlen,buf,len);
        tlen += len;
        p++;
        continue;
      }
    }
    /* Select the next token type according to its type specifier. */
    if (*p == ']') p++; /* Skip closing "]", it's just useless syntax. */
    if (*p == '\0') break;
    else if (*p == '.') next = JSEL_OBJ;
    else if (*p == '[') next = JSEL_ARRAY;
    else if (*p == ':') next = JSEL_TYPECHECK;
    else goto notfound;
    tlen = 0; /* A new token starts. */
    p++; /* Token starts at next character. */
  }

cleanup:
  va_end(ap);
  return o;

notfound:
  o = NULL;
  goto cleanup;
}


int cee_json_select_as_int(struct cee_json *o, int *x, char *fmt){
  struct cee_json *i = cee_json_select(o, fmt);
  if( i )
    return cee_json_to_intx(i, x);
  return 1;
}

size_t cee_json_escape_string(const char *s, size_t s_size,
                              char *dest, size_t dest_size,
                              char **next_dest_p, size_t *next_dest_size){
  int j = 0, c, i;
  for( i = 0; i < s_size; i++ ){
    c = s[i];
    switch( c ){
    case '"' : dest[j] = '\\', dest[j+1] = '"', j+= 2; break;
    case '\\': dest[j] = '\\', dest[j+1] = '\\', j+= 2; break;
    case '\b': dest[j] = '\\', dest[j+1] = 'b', j+= 2; break;
    case '\f': dest[j] = '\\', dest[j+1] = 'f', j+= 2; break;
    case '\n': dest[j] = '\\', dest[j+1] = 'n', j+= 2; break;
    case '\r': dest[j] = '\\', dest[j+1] = 'r', j+= 2; break;
    case '\t': dest[j] = '\\', dest[j+1] = 't', j+= 2; break;
    default:
      if( '\x00' <= c && c <= '\x1f' ){
        dest[j] = '\\', dest[j+1] = 'u';
        snprintf(dest+j+2, dest_size - j - 2, "%04x", c);
        j += 4;
      }else{
        dest[j] = c, j++;
      }
    }
  }
  if( next_dest_size )
    *next_dest_size = dest_size - j;

  if( next_dest_p )
    *next_dest_p = dest + j;

  return j;
}
/* cee_json parser
   C reimplementation of cppcms's json.cpp
*/
enum state_type {
  st_init = 0,
  st_object_or_array_or_value_expected = 0 ,
  st_object_key_or_close_expected,
  st_object_colon_expected,
  st_object_value_expected,
  st_object_close_or_comma_expected,
  st_array_value_or_close_expected,
  st_array_close_or_comma_expected,
  st_error,
  st_done
} state_type;


static const uintptr_t cee_json_max_depth = 512;




int cee_json_parsex(struct cee_state * st, char * buf, uintptr_t len, struct cee_json **out, bool force_eof,
      int *error_at_line)
{
  struct tokenizer tock = {0};
  tock.buf = buf;
  tock.buf_end = buf + len;
  *out = NULL;

  enum state_type state = st_init;
  struct cee_str * key = NULL;

  struct cee_stack * sp = cee_stack_mk_e(st, CEE_DP_NOOP, cee_json_max_depth);
  struct cee_tuple * top = NULL;
  struct cee_tuple * result = NULL;





  cee_stack_push(sp, cee_tuple_mk_e(st, (enum cee_del_policy [2]){CEE_DP_NOOP, CEE_DP_NOOP}, (void *)st_done, NULL));

  while(!cee_stack_empty(sp) && !cee_stack_full(sp)
          && state != st_error && state != st_done)
  {
    if (result) {
      cee_del(result);
      result = NULL;
    }

    int c = cee_json_next_token(st, &tock);




    top = (struct cee_tuple *)cee_stack_top(sp, 0);
    switch(state) {
    case st_object_or_array_or_value_expected:
        if(c=='[') {
          top->_[1]=cee_json_array_mk(st, 20);
          state=st_array_value_or_close_expected;
        }
        else if(c=='{') {
          top->_[1]=cee_json_object_mk(st);
          state=st_object_key_or_close_expected;
        }
        else if(c==tock_str) {
          top->_[1]=cee_json_str_mk(st, tock.str);
          tock.str = NULL;
          state=(enum state_type)(top->_[0]);
          do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0);
        }
        else if(c==tock_true) {
          top->_[1]=cee_json_true();
          state=(enum state_type)(top->_[0]);
          do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0);
        }
        else if(c==tock_false) {
          top->_[1]=cee_json_false();
          state=(enum state_type)(top->_[0]);
          do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0);
        }
        else if(c==tock_null) {
          top->_[1]=cee_json_null();
          state=(enum state_type)(top->_[0]);
          do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0);
        }
        else if(c==tock_number) {
          if (tock.type == NUMBER_IS_I64)
            top->_[1] = cee_json_i64_mk(st, tock.number.i64);
          else if (tock.type == NUMBER_IS_U64)
            top->_[1] = cee_json_u64_mk(st, tock.number.u64);
          else
            top->_[1] = cee_json_double_mk(st, tock.number.real);
          state=(enum state_type)(top->_[0]);
          do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0);
        }
        else
          state = st_error;
        break;
    case st_object_key_or_close_expected:
        if(c=='}') {
          state=(enum state_type)(top->_[0]);
          do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0);
        }
        else if (c==tock_str) {
          key = tock.str;
          tock.str = NULL;
          state = st_object_colon_expected;
        }
        else
          state = st_error;
        break;
    case st_object_colon_expected:
        if(c!=':')
          state=st_error;
        else
          state=st_object_value_expected;
        break;
    case st_object_value_expected: {
        struct cee_map * obj = cee_json_to_object(top->_[1]);
        if(c==tock_str) {
          cee_map_add(obj, key, cee_json_str_mk(st, tock.str));
          tock.str = NULL;
          state=st_object_close_or_comma_expected;
        }
        else if(c==tock_true) {
          cee_map_add(obj, key, cee_json_true());
          state=st_object_close_or_comma_expected;
        }
        else if(c==tock_false) {
          cee_map_add(obj, key, cee_json_false());
          state=st_object_close_or_comma_expected;
        }
        else if(c==tock_null) {
          cee_map_add(obj, key, cee_json_null());
          state=st_object_close_or_comma_expected;
        }
        else if(c==tock_number) {
          if (tock.type == NUMBER_IS_I64)
            cee_map_add(obj, key, cee_json_i64_mk(st, tock.number.i64));
          else if (tock.type == NUMBER_IS_U64)
            cee_map_add(obj, key, cee_json_u64_mk(st, tock.number.u64));
          else
            cee_map_add(obj, key, cee_json_double_mk(st, tock.number.real));
          state=st_object_close_or_comma_expected;
        }
        else if(c=='[') {
          struct cee_json * a = cee_json_array_mk(st, 20);
          cee_map_add(obj, key, a);
          state=st_array_value_or_close_expected;
          cee_stack_push(sp, cee_tuple_mk_e(st, (enum cee_del_policy [2]){CEE_DP_NOOP, CEE_DP_NOOP}, (void *)st_object_close_or_comma_expected, a));
        }
        else if(c=='{') {
          struct cee_json * o = cee_json_object_mk(st);
          cee_map_add(obj, key, o);
          state=st_object_key_or_close_expected;
          cee_stack_push(sp, cee_tuple_mk_e(st, (enum cee_del_policy [2]){CEE_DP_NOOP, CEE_DP_NOOP}, (void *)st_object_close_or_comma_expected, o));
        }
        else
          state=st_error;
        break; }
    case st_object_close_or_comma_expected:
        if(c==',')
          state=st_object_key_or_close_expected;
        else if(c=='}') {
          state=(enum state_type)(top->_[0]);
          do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0);
        }
        else
          state=st_error;
        break;
    case st_array_value_or_close_expected: {
        if(c==']') {
          state=(enum state_type)(top->_[0]);
          do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0);
          break;
        }
        struct cee_list * ar = cee_json_to_array(top->_[1]);

        if(c==tock_str) {
          cee_list_append(ar, cee_json_str_mk(st, tock.str));
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_true) {
          cee_list_append(ar, cee_json_true());
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_false) {
          cee_list_append(ar, cee_json_false());
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_null) {
          cee_list_append(ar, cee_json_null());
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_number) {
          if (tock.type == NUMBER_IS_I64)
            cee_list_append(ar, cee_json_i64_mk(st, tock.number.i64));
          else if (tock.type == NUMBER_IS_U64)
            cee_list_append(ar, cee_json_u64_mk(st, tock.number.u64));
          else
            cee_list_append(ar, cee_json_double_mk(st, tock.number.real));
          state=st_array_close_or_comma_expected;
        }
        else if(c=='[') {
          struct cee_json * a = cee_json_array_mk(st, 20);
          cee_list_append(ar, a);
          state=st_array_value_or_close_expected;
          cee_stack_push(sp, cee_tuple_mk_e(st, (enum cee_del_policy [2]){CEE_DP_NOOP, CEE_DP_NOOP}, (void *)st_array_close_or_comma_expected, a));
        }
        else if(c=='{') {
          struct cee_json * o = cee_json_object_mk(st);
          cee_list_append(ar, o);
          state=st_object_key_or_close_expected;
          cee_stack_push(sp, cee_tuple_mk_e(st, (enum cee_del_policy [2]){CEE_DP_NOOP, CEE_DP_NOOP}, (void *)st_array_close_or_comma_expected, o));
        }
        else
          state=st_error;
        break; }
    case st_array_close_or_comma_expected:
        if(c==']') {
          state=(enum state_type)(top->_[0]);
          do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0);
        }
        else if(c==',')
          state=st_array_value_or_close_expected;
        else
          state=st_error;
        break;
    case st_done:
    case st_error:
        break;
    };
  }

  cee_del(sp);
  if(state==st_done) {
    if(force_eof) {
      if(cee_json_next_token(st, &tock)!=tock_eof) {
        *error_at_line=tock.line;
        return EINVAL;
      }
    }
    *out = (struct cee_json *)(result->_[1]);
    cee_del(result);
    return 0;
  }
  *error_at_line=tock.line;
  return EINVAL;
}
/* JSON snprint
   C reimplementation of cppcms's json.cpp
*/







struct counter {
  uintptr_t next;
  struct cee_list * array;
  struct cee_map * object;
  char tabs;
  char more_siblings;
};

static struct counter * push(struct cee_state * st, uintptr_t tabs, bool more_siblings,
                             struct cee_stack * sp, struct cee_json * j) {
  struct counter * p = NULL;
  if (j == NULL) {
    p = cee_block_mk_nonzero(st, sizeof(struct counter));
    p->tabs = 0;
  }
  else {
    switch(j->t) {
      case CEE_JSON_OBJECT:
        {
          p = cee_block_mk_nonzero(st, sizeof(struct counter));
          struct cee_map * mp = cee_json_to_object(j);
          p->array = cee_map_insertion_ordered_keys(mp);
          p->object = cee_json_to_object(j);
          p->tabs = tabs;
          p->next = 0;
          p->more_siblings = 0;
        }
        break;
      case CEE_JSON_ARRAY:
        {
          p = cee_block_mk_nonzero(st, sizeof(struct counter));
          p->array = cee_json_to_array(j);
          p->tabs = tabs;
          p->next = 0;
          p->more_siblings = 0;
        }
        break;
      default:
        {
          p = cee_block_mk_nonzero(st, sizeof(struct counter));
          p->array = NULL;
          p->tabs = tabs;
          p->next = 0;
          p->more_siblings = 0;
        }
        break;
    }
    p->more_siblings = more_siblings;
  }
  enum cee_del_policy o[2] = { CEE_DP_DEL, CEE_DP_NOOP };
  cee_stack_push(sp, cee_tuple_mk_e(st, o, p, j));
  return p;
}

static void pad (uintptr_t * offp, char * buf, struct counter * cnt,
                 enum cee_json_fmt f) {
  if (!f) return;

  uintptr_t offset = *offp;
  if (buf) {
    int i;
    for (i = 0; i < cnt->tabs; i++)
      buf[offset + i] = '\t';
  }
  offset += cnt->tabs;
  *offp = offset;
  return;
}

static void delimiter (uintptr_t * offp, char * buf, enum cee_json_fmt f,
                       struct counter * cnt, char c)
{
  uintptr_t offset = *offp;
  if (!f) {
    if (buf) buf[offset] = c;
    offset ++; /*  only count one */
    *offp = offset;
    return;
  }

  switch (c) {
    case '[':
    case '{':
      pad(offp, buf, cnt, f);
      if (buf) {
        buf[offset] = c;
        buf[offset+1] = '\n';
      }
      offset +=2;
      break;
    case ']':
    case '}':
      if (buf) buf[offset] = '\n';
      offset ++;
      pad(&offset, buf, cnt, f);
      if (buf) buf[offset] = c;
      offset ++;
      if (buf) buf[offset] = '\n';
      offset ++;
      break;
    case ':':
      if (buf) {
        buf[offset] = ' ';
        buf[offset+1] = ':';
        buf[offset+2] = '\t';
      }
      offset +=3;
      break;
    case ',':
      if (buf) {
        buf[offset] = ',';
        buf[offset+1] = '\n';
      }
      offset +=2;
      break;
  }
  *offp = offset;
}


static void str_append(char * out, uintptr_t *offp, char *begin, unsigned len) {
  uintptr_t offset = *offp;

  if (out) out[offset] = '"';
  offset ++;

  char *i,*last;
  char buf[8] = "\\u00";
  for(i=begin,last = begin;i < begin + len;) {
    char *addon = 0;
    unsigned char c=*i;
    switch(c) {
    case 0x22: addon = "\\\""; break;
    case 0x5C: addon = "\\\\"; break;
    case '\b': addon = "\\b"; break;
    case '\f': addon = "\\f"; break;
    case '\n': addon = "\\n"; break;
    case '\r': addon = "\\r"; break;
    case '\t': addon = "\\t"; break;
    default:
      if(c<=0x1F) {
        static char const tohex[]="0123456789abcdef";
        buf[4]=tohex[c >> 4];
        buf[5]=tohex[c & 0xF];
        buf[6]=0;
        addon = buf;
      }
    };
    if(addon) {
      /* a.append(last,i-last); */
      if (out) memcpy(out+offset, last, i-last);
      offset += i-last;

      if (out) memcpy(out+offset, addon, strlen(addon));
      offset += strlen(addon);
      i++;
      last = i;
    }
    else {
      i++;
    }
  }
  if (out) memcpy(out+offset, last, i-last);
  offset += i-last;
  if (out) out[offset] = '"';
  offset++;
  *offp = offset;
}

/*
 * compute how many bytes are needed to serialize cee_json as a string
 */
ssize_t cee_json_snprint (struct cee_state *st, char *buf, size_t size, struct cee_json *j,
     enum cee_json_fmt f) {
  struct cee_tuple * cur;
  struct cee_json * cur_json;
  struct counter * ccnt;
  uintptr_t incr = 0;

  struct cee_stack *sp = cee_stack_mk_e(st, CEE_DP_NOOP, 500);
  push (st, 0, false, sp, j);

  uintptr_t offset = 0;
  while (!cee_stack_empty(sp) && !cee_stack_full(sp)) {
    cur = cee_stack_top(sp, 0);
    cur_json = (struct cee_json *)(cur->_[1]);
    ccnt = (struct counter *)(cur->_[0]);

    switch(cur_json->t) {
      case CEE_JSON_NULL:
        {
          pad(&offset, buf, ccnt, f);
          if (buf)
            memcpy(buf + offset, "null", 4);
          offset += 4;
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_BOOLEAN:
        {
          pad(&offset, buf, ccnt, f);
          char * s = "false";
          bool b;
          cee_json_to_boolx(cur_json, &b);
          if (b)
            s = "true";
          if (buf)
            memcpy(buf + offset, s, strlen(s));
          offset += strlen(s);
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_UNDEFINED:
        {
          pad(&offset, buf, ccnt, f);
          if (buf)
            memcpy(buf + offset, "undefined", 9);
          offset += 9;
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_STRING:
        {
          char *str = (char *)cee_json_to_str(cur_json);
          pad(&offset, buf, ccnt, f);
          /* TODO: escape str */
          str_append(buf, &offset, str, strlen(str));
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_STRN:
        {
          char *str;
          size_t slen;
          cee_json_to_strn(cur_json, &str, &slen);
          pad(&offset, buf, ccnt, f);
          str_append(buf, &offset, str, slen);
          if( ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_BLOB:
        {
          /* TODO: encode as base64 */
          struct cee_block *blob = cee_json_to_blob(cur_json);
          char *str = (char*)cee_str_mk(st, "blob:%d bytes", cee_block_size(blob));
          pad(&offset, buf, ccnt, f);
          str_append(buf, &offset, str, strlen(str));
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_DOUBLE:
      case CEE_JSON_I64:
      case CEE_JSON_U64:
        {
          pad(&offset, buf, ccnt, f);
          incr = cee_boxed_snprint (NULL, 0, cee_json_to_boxed(cur_json));
          if (buf)
            cee_boxed_snprint (buf+offset, incr+1/*\0*/, cee_json_to_boxed(cur_json));
          offset+=incr;
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_ARRAY:
        {
          uintptr_t i = ccnt->next;
          if (i == 0)
            delimiter(&offset, buf, f, ccnt, '[');

          uintptr_t n = cee_list_size(ccnt->array);
          if (i < n) {
            bool more_siblings = false;
            if (1 < n && i+1 < n)
              more_siblings = true;
            ccnt->next++;
            push (st, ccnt->tabs + 1, more_siblings, sp,
                  (struct cee_json *)(ccnt->array->a[i]));
          }
          else {
            delimiter(&offset, buf, f, ccnt, ']');
            if (ccnt->more_siblings)
              delimiter(&offset, buf, f, ccnt, ',');
            cee_del(cee_stack_pop(sp));
          }
        }
        break;
      case CEE_JSON_OBJECT:
        {
          uintptr_t i = ccnt->next;
          if (i == 0)
            delimiter(&offset, buf, f, ccnt, '{');

          uintptr_t n = cee_list_size(ccnt->array);
          if (i < n) {
            bool more_siblings = false;
            if (1 < n && i+1 < n)
              more_siblings = true;

            ccnt->next++;
            char * key = (char *)ccnt->array->a[i];
            struct cee_json * j1 = cee_map_find(ccnt->object, ccnt->array->a[i]);
            unsigned klen = strlen(key);
            pad(&offset, buf, ccnt, f);
            str_append(buf, &offset, key, klen);
            delimiter(&offset, buf, f, ccnt, ':');
            push(st, ccnt->tabs + 1, more_siblings, sp, j1);
          }
          else {
            delimiter(&offset, buf, f, ccnt, '}');
            if (ccnt->more_siblings)
              delimiter(&offset, buf, f, ccnt, ',');
            cee_del(ccnt->array);
            cee_del(cee_stack_pop(sp));
          }
        }
        break;
      default:
        cee_segfault();
    }
  }
  cee_del (sp);
  if (buf)
    buf[offset] = '\0';
  return offset;
}


/*
 * the memory allocated by this function is tracked by cee_state
 */
ssize_t
cee_json_asprint(struct cee_state *st, char **buf_p, size_t *buf_size_p, struct cee_json *j,
                 enum cee_json_fmt f)
{
  size_t buf_size = cee_json_snprint(st, NULL, 0, j, f);
  char *buf = cee_block_mk_nonzero(st, buf_size);
  if (buf_size_p) *buf_size_p = buf_size;
  *buf_p = buf;
  return cee_json_snprint(st, buf, buf_size, j, f);
}


void
cee_json_dprintf(int fd, struct cee_json *json, const char *fmt, ...){
  struct cee_state *_st = cee_state_mk(10);
  char *_1p = NULL; size_t _1s = 0;
  if( json )
    cee_json_asprint(_st, &_1p, &_1s, json, 0);

  char *new_fmt;
  if( fmt ){
    va_list ap;
    va_start(ap, fmt);
    vdprintf(fd, fmt, ap);
    va_end(ap);
  }
  dprintf(fd, "%s\n", _1p ? _1p:"null");
  cee_del(_st);
}








static const uint32_t utf_illegal = 0xFFFFFFFFu;
static bool utf_valid(uint32_t v)
{
  if(v>0x10FFFF)
    return false;
  if(0xD800 <=v && v<= 0xDFFF) /*  surragates */
    return false;
  return true;
}

/* namespace utf8 { */
static bool utf8_is_trail(char ci)
{
  unsigned char c=ci;
  return (c & 0xC0)==0x80;
}


static int utf8_trail_length(unsigned char c)
{
  if(c < 128)
    return 0;
  if(c < 194)
    return -1;
  if(c < 224)
    return 1;
  if(c < 240)
    return 2;
  if(c <=244)
    return 3;
  return -1;
}

static int utf8_width(uint32_t value)
{
  if(value <=0x7F) {
    return 1;
  }
  else if(value <=0x7FF) {
    return 2;
  }
  else if(value <=0xFFFF) {
    return 3;
  }
  else {
    return 4;
  }
}

/*  See RFC 3629 */
/*  Based on: http://www.w3.org/International/questions/qa-forms-utf-8 */
static uint32_t next(char ** p, char * e, bool html)
{
  if(*p==e)
    return utf_illegal;

  unsigned char lead = **p;
  (*p)++;

  /*  First byte is fully validated here */
  int trail_size = utf8_trail_length(lead);

  if(trail_size < 0)
    return utf_illegal;

  /*  */
  /*  Ok as only ASCII may be of size = 0 */
  /*  also optimize for ASCII text */
  /*  */
  if(trail_size == 0) {
    if(!html || (lead >= 0x20 && lead!=0x7F) || lead==0x9 || lead==0x0A || lead==0x0D)
      return lead;
    return utf_illegal;
  }

  uint32_t c = lead & ((1<<(6-trail_size))-1);

  /*  Read the rest */
  unsigned char tmp;
  switch(trail_size) {
    case 3:
      if(*p==e)
        return utf_illegal;
      tmp = **p;
      (*p)++;
      if (!utf8_is_trail(tmp))
        return utf_illegal;
      c = (c << 6) | ( tmp & 0x3F);
    case 2:
      if(*p==e)
        return utf_illegal;
      tmp = **p;
      (*p)++;
      if (!utf8_is_trail(tmp))
        return utf_illegal;
      c = (c << 6) | ( tmp & 0x3F);
    case 1:
      if(*p==e)
        return utf_illegal;
      tmp = **p;
      (*p)++;
      if (!utf8_is_trail(tmp))
        return utf_illegal;
      c = (c << 6) | ( tmp & 0x3F);
  }

  /*  Check code point validity: no surrogates and */
  /*  valid range */
  if(!utf_valid(c))
    return utf_illegal;

  /*  make sure it is the most compact representation */
  if(utf8_width(c)!=trail_size + 1)
    return utf_illegal;

  if(html && c<0xA0)
    return utf_illegal;
  return c;
} /*  valid */


/*
bool validate_with_count(char * p, char * e, size_t *count,bool html)
{
  while(p!=e) {
    if(next(p,e,html)==utf_illegal)
      return false;
    (*count)++;
  }
  return true;
}
*/

static bool utf8_validate(char * p, char * e)
{
  while(p!=e)
    if(next(&p, e, false)==utf_illegal)
      return false;
  return true;
}


struct utf8_seq {
  char c[4];
  unsigned len;
};

static void utf8_encode(uint32_t value, struct utf8_seq *out) {
  /* struct utf8_seq out={0}; */
  if(value <=0x7F) {
    out->c[0]=value;
    out->len=1;
  }
  else if(value <=0x7FF) {
    out->c[0]=(value >> 6) | 0xC0;
    out->c[1]=(value & 0x3F) | 0x80;
    out->len=2;
  }
  else if(value <=0xFFFF) {
    out->c[0]=(value >> 12) | 0xE0;
    out->c[1]=((value >> 6) & 0x3F) | 0x80;
    out->c[2]=(value & 0x3F) | 0x80;
    out->len=3;
  }
  else {
    out->c[0]=(value >> 18) | 0xF0;
    out->c[1]=((value >> 12) & 0x3F) | 0x80;
    out->c[2]=((value >> 6) & 0x3F) | 0x80;
    out->c[3]=(value & 0x3F) | 0x80;
    out->len=4;
  }
}

static bool check(char * buf, char * s, char **ret)
{
  char * next = buf;

  for (next = buf; *s && *next == *s; next++, s++);
  if (*s==0) {
    *ret = next;
    return true;
  }
  else {
    *ret = buf;
   return false;
  }
  return false;
}

static bool read_4_digits(char ** str_p, char * const buf_end, uint16_t *x)
{
  char * str = * str_p;
  if (buf_end - str < 4)
    return false;

  char buf[5] = { 0 };
  int i;
  for(i=0; i<4; i++) {
    char c=str[i];
    buf[i] = c;
    if(isxdigit(c))
      continue;

    return false;
  }
  unsigned v;
  sscanf(buf,"%x",&v);
  *x=v;
  *str_p = str + 4;
  return true;
}

static int utf16_is_first_surrogate(uint16_t x)
{
  return 0xD800 <=x && x<= 0xDBFF;
}

static int utf16_is_second_surrogate(uint16_t x)
{
  return 0xDC00 <=x && x<= 0xDFFF;
}

static uint32_t utf16_combine_surrogate(uint16_t w1,uint16_t w2)
{
  return ((((uint32_t)w1 & 0x3FF) << 10) | (w2 & 0x3FF)) + 0x10000;
}

static void * append (uint32_t x, char *d)
{
  struct utf8_seq seq = { {0}, 0 };
  unsigned int i;

  utf8_encode(x, &seq);
  for (i = 0; i < seq.len; ++i, d++)
    *d = seq.c[i];
  return d;
}

static int
json_string_unescape(char **output_p, size_t *output_len_p,
                     char *input, size_t input_len)
{
  unsigned char c;
  char * const input_start = input, * const input_end = input + input_len;
  char * out_start = NULL, * d = NULL, * s = NULL;
  uint16_t first_surrogate;
  int second_surrogate_expected;


  enum state {
    TESTING = 1,
    ALLOCATING,
    UNESCAPING,
  } state = TESTING;

second_iter:
  first_surrogate = 0;
  second_surrogate_expected = 0;
  for (s = input_start; s < input_end;) {
    c = * s;
    s ++;

    if (second_surrogate_expected && c != '\\')
      goto return_err;

    if (0<= c && c <= 0x1F)
      goto return_err;

    if('\\' == c) {
      if (TESTING == state) {
        state = ALLOCATING;
        break; /*  break the while loop */
      }

      if (s == input_end) {
        /* input is not a well-formed json string */
        goto return_err;
      }

      c = * s;
      s ++;

      if (second_surrogate_expected && c != 'u')
        goto return_err;

      switch(c) {
      case '"':
      case '\\':
      case '/':
          *d = c; d++; break;
      case 'b': *d = '\b'; d ++; break;
      case 'f': *d = '\f'; d ++; break;
      case 'n': *d = '\n'; d ++; break;
      case 'r': *d = '\r'; d ++; break;
      case 't': *d = '\t'; d ++; break;
      case 'u': {
          uint16_t x;
          if (!read_4_digits(&s, input_end, &x))
            goto return_err;
          if (second_surrogate_expected) {
            if (!utf16_is_second_surrogate(x))
              goto return_err;
            d = append(utf16_combine_surrogate(first_surrogate, x), d);
            second_surrogate_expected = 0;
          } else if (utf16_is_first_surrogate(x)) {
            second_surrogate_expected = 1;
            first_surrogate = x;
          } else {
            d = append(x, d);
          }
          break; }
      default:
          goto return_err;
      }
    }
    else if (UNESCAPING == state) {
      *d = c;
      d++;
    }
  }

  switch (state) {
  case UNESCAPING:
      if (!utf8_validate(out_start, d))
        goto return_err;
      else
      {
        *output_p = out_start;
        *output_len_p = d - out_start;
        return 1;
      }
  case ALLOCATING:
      out_start = calloc(1, input_len);
      d = out_start;
      state = UNESCAPING;
      goto second_iter;
  case TESTING:
      *output_p = input_start;
      *output_len_p = input_len;
      return 1;
  default:
      break;
  }

return_err:
  if (UNESCAPING == state)
    free(out_start);
  return 0;
}

static bool parse_string(struct cee_state * st, struct tokenizer * t) {
  char *start = t->buf + 1; /*  start after initial '"' */
  char *end = start;

  /*  reach the end of the string */
  while (*end != '\0' && *end != '"') {
    if ('\\' == *end++ && *end != '\0') { /*  check for escaped characters */
      ++end; /*  eat-up escaped character */
    }
  }
  if (*end != '"') return false; /*  make sure reach end of string */

  char * unscp_str = NULL;
  size_t unscp_len = 0;
  if (json_string_unescape(&unscp_str, &unscp_len, start, end-start)) {
    if (unscp_str == start) {
      t->str = cee_str_mk_e(st, end-start+1, "%.*s", end-start, start);
    }
    else {
      /* / @todo? create a cee_str func that takes ownership of string */
      t->str = cee_str_mk_e(st, unscp_len+1, "%s", unscp_str);
      free(unscp_str);
    }
    t->buf = end + 1; /*  skip the closing '"' */
    return true;
  }
  return false; /*  ill formed string */
}

static bool parse_number(struct tokenizer *t) {
  char *start = t->buf;
  char *end = start;

  bool is_integer = true, is_exponent = false;
  int offset_sign = 0;

  /* 1st STEP: check for a minus sign and skip it */
  if ('-' == *end) {
    ++end;
    offset_sign = 1;
  }
  if (!isdigit(*end))
    return false;

  /* 2nd STEP: skips until a non digit char found */
  while (isdigit(*++end))
    continue;

  /* 3rd STEP: if non-digit char is not a comma then it must be an integer */
  if ('.' == *end) {
    if (!isdigit(*++end))
      return false;
    while (isdigit(*++end))
      continue;
    is_integer = false;
  }

  /* 4th STEP: check for exponent and skip its tokens */
  /*  TODO: check if its an integer or not and change 'is_integer' accordingly */
  if ('e' == *end || 'E' == *end) {
    ++end;
    if ('+' == *end || '-' == *end)
      ++end;
    if (!isdigit(*end))
      return false;
    while (isdigit(*++end))
      continue;
    is_exponent = true;
  } /*  can't be a integer that starts with zero followed n numbers (ex: 012, -023) */
  else if (is_integer && (end-1) != (start+offset_sign) && '0' == start[offset_sign]) {
    return false;
  }

  /* 5th STEP: convert string to number */
  char *endptr=NULL;
  if (is_exponent || !is_integer) {
    t->type = NUMBER_IS_DOUBLE;
    t->number.real = strtod(start, &endptr);
  }
  else {
    t->type = NUMBER_IS_I64;
    t->number.i64 = strtoll(start, &endptr, 10);
  }

  t->buf = end; /* skips entire length of number */

  return start != endptr;
}

enum token cee_json_next_token(struct cee_state * st, struct tokenizer * t) {
  for (;;t->buf++) {
    if (t->buf >= t->buf_end)
      return tock_eof;
    char c = t->buf[0];
    switch (c) {
    case '[':
    case '{':
    case ':':
    case ',':
    case '}':
    case ']':
        t->buf++;
        return c;
    case '\n':
    case '\r':
        t->line++;
    /* fallthrough */
    case ' ':
    case '\t':
        break;
    case '\"':
        if(parse_string(st, t))
          return tock_str;
        return tock_err;
    case 't':
        t->buf++;
        if(check(t->buf, "rue", &t->buf))
          return tock_true;
        return tock_err;
    case 'n':
        t->buf++;
        if(check(t->buf, "ull", &t->buf))
          return tock_null;
        return tock_err;
    case 'f':
        t->buf++;
        if(check(t->buf, "alse", &t->buf))
          return tock_false;
        return tock_err;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        if(parse_number(t))
          return tock_number;
        return tock_err;
    case '/':
        t->buf++;
        if(check(t->buf, "/", &t->buf)) {
          while(t->buf < t->buf_end) {
            c = t->buf[0];
            if(c=='\0') return tock_eof;
            if(c=='\n') return tock_err;
            t->buf++;
          }
        }
        return tock_err;
    default:
        return tock_err;
    }
  }
}






int cee_json_merge_all(char **json_files, int json_file_count, char *merged){
  static char perror_buf[128] = {0};

  FILE *f0 = fopen(json_files[0], "r"), *fn, *f_combined;
  if( !f0 ){
    snprintf(perror_buf, sizeof perror_buf, "fopen(%s)", json_files[0]);
    perror(perror_buf);
    return 1;
  }
  struct cee_state *state = cee_state_mk(100);
  int line = -1;
  struct cee_json *j0, *jn;
  j0 = cee_json_load_from_FILE(state, f0, true, &line);
  if( j0 == NULL ){
    fprintf(stderr, "error at %s:%d\n", json_files[0], line);
    return 1;
  }

  for( int i = 1; i < json_file_count; i++ ){
    fn = fopen(json_files[i], "r");
    if( !fn ){
      snprintf(perror_buf, sizeof perror_buf, "fopen(%s)", json_files[i]);
      perror(perror_buf);
      return 1;
    }
    line = -1;
    jn = cee_json_load_from_FILE(state, fn, true, &line);
    if( jn == NULL ){
      fprintf(stderr, "error at %s:%d\n", json_files[i], line);
      return 1;
    }
    cee_json_merge(j0, jn);
    fclose(fn);
  }

  f_combined = fopen(merged, "w");
  if( !f_combined ){
    perror(merged);
    return 1;
  }
  struct cee_json *inputs = cee_json_array_mk(state, json_file_count);
  cee_json_object_set(j0, "originals", inputs);
  for( int i = 0; i < json_file_count; i++ ){
    struct cee_json *a = cee_json_str_mkf(state, "%s", json_files[i]);
    cee_json_array_append(inputs, a);
  }

  cee_json_save(state, j0, f_combined, 1);
  fclose(f_combined);
  fclose(f0);
  cee_del(state);
  return 0;
}

#endif
