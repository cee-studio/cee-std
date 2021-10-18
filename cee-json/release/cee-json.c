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

#define MAX_JSON_DEPTH 500

enum cee_json_type {
  CEE_JSON_UNDEFINED,    /**<  Undefined value */
  CEE_JSON_NULL,         /**<  null value */
  CEE_JSON_BOOLEAN,      /**<  boolean value */
  CEE_JSON_DOUBLE,       /**<  double value */
  CEE_JSON_I64,          /**<  64-bit signed int */
  CEE_JSON_U64,          /**<  64-bit unsigned int */
  CEE_JSON_STRING,       /**<  string value */
  CEE_JSON_OBJECT,       /**<  object value  */
  CEE_JSON_ARRAY         /**<  array value */
};

struct cee_json {
  enum cee_json_type t;
  union {
    struct cee_singleton * null;
    struct cee_singleton * undefined;
    struct cee_singleton * boolean;
    struct cee_boxed     * boxed;
    struct cee_str       * string;
    struct cee_list      * array;
    struct cee_map       * object;
  } value;
};

enum cee_json_fmt {
  CEE_JSON_FMT_COMPACT = 0,
  CEE_JSON_FMT_PRETTY  = 1
};

/*
 *
 */
extern struct cee_json* cee_json_select (struct cee_json *, char *selector, ...);

extern bool cee_json_save (struct cee_state *, struct cee_json *, FILE *, int how);
extern struct cee_json * cee_json_load_from_file (struct cee_state *,
                                                  FILE *, bool force_eof, 
                                                  int * error_at_line);
extern struct cee_json * cee_json_load_from_buffer (int size, char *, int line);
extern int cee_json_cmp (struct cee_json *, struct cee_json *);


extern struct cee_json * cee_list_to_json (struct cee_list *v);
extern struct cee_json * cee_map_to_json (struct cee_map *v);

extern struct cee_list * cee_json_to_array (struct cee_json *);
extern struct cee_map * cee_json_to_object (struct cee_json *);
extern struct cee_boxed * cee_json_to_boxed (struct cee_json *);
extern struct cee_str* cee_json_to_str (struct cee_json *);
extern double cee_json_to_double (struct cee_json *);
extern int64_t cee_json_to_i64 (struct cee_json*);
extern uint64_t cee_json_to_u64 (struct cee_json*);
extern bool cee_json_to_bool(struct cee_json*);

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
extern struct cee_json * cee_json_str_mk (struct cee_state *, struct cee_str * s);
extern struct cee_json * cee_json_str_mkf (struct cee_state *, const char *fmt, ...);
extern struct cee_json * cee_json_array_mk (struct cee_state *, int s);

extern void cee_json_object_set (struct cee_json *, char *, struct cee_json *);
extern void cee_json_object_set_bool (struct cee_json *, char *, bool);
extern void cee_json_object_set_str (struct cee_json *, char *, char *);
extern void cee_json_object_set_strf (struct cee_json *, char *, const char *fmt, ...);
extern void cee_json_object_set_double (struct cee_json *, char *, double);
extern void cee_json_object_set_i64 (struct cee_json *, char *, int64_t);
extern void cee_json_object_set_u64 (struct cee_json *, char *, uint64_t);


extern struct cee_json* cee_json_object_get(struct cee_json *, char *key);
/* remove a key from a json object */
extern void cee_json_object_remove (struct cee_json *, char *);

extern void cee_json_object_iterate (struct cee_json *, void *ctx, 
                                     void (*f)(void *ctx, struct cee_str *key, struct cee_json *val));


extern void cee_json_array_append (struct cee_json *, struct cee_json *);
extern void cee_json_array_append_bool (struct cee_json *, bool);
extern void cee_json_array_append_str (struct cee_json *, char *);
extern void cee_json_array_append_strf (struct cee_json *, const char *fmt, ...);
extern void cee_json_array_append_double (struct cee_json *, double);
extern void cee_json_array_append_i64 (struct cee_json *, int64_t);
extern void cee_json_array_append_u64 (struct cee_json *, uint64_t);

/* remove an element from a json array */
extern void cee_json_array_remove (struct cee_json *, int index);

extern struct cee_json* cee_json_array_get(struct cee_json *, int);
extern void cee_json_array_iterate (struct cee_json *, void *ctx,
				    void (*f)(void *ctx, int index, struct cee_json *val));

extern ssize_t cee_json_snprint (struct cee_state *, char *buf,
				 size_t size, struct cee_json *json,
				 enum cee_json_fmt);

extern ssize_t cee_json_asprint (struct cee_state *, char **buf_p,
				 struct cee_json *json, enum cee_json_fmt);

extern bool cee_json_parse(struct cee_state *st, char *buf, uintptr_t len, struct cee_json **out, 
                           bool force_eof, int *error_at_line);

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

  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_OBJECT, v);
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

struct cee_boxed * cee_json_to_boxed (struct cee_json *p) {
  if (p->t == CEE_JSON_U64 || p->t == CEE_JSON_I64 || p->t == CEE_JSON_DOUBLE)
    return p->value.boxed;
  else
    return NULL;
}

double cee_json_to_double (struct cee_json *p) {
  if (p->t == CEE_JSON_DOUBLE)
    return cee_boxed_to_double(p->value.boxed);
  else
    cee_segfault();
}

int64_t cee_json_to_i64 (struct cee_json *p) {
  if (p->t == CEE_JSON_I64)
    return cee_boxed_to_i64(p->value.boxed);
  else
    cee_segfault();
}

uint64_t cee_json_to_u64 (struct cee_json *p) {
  if (p->t == CEE_JSON_U64)
    return cee_boxed_to_u64(p->value.boxed);
  else
    cee_segfault();
}

bool cee_json_to_bool(struct cee_json *p) {
  if (p == cee_json_true())
    return true;
  else if (p == cee_json_false())
    return false;
  else
    cee_segfault();
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

struct cee_json* cee_json_object_get(struct cee_json *j, char *key)
{
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();
  return cee_map_find(o, key);
}

void cee_json_object_remove(struct cee_json *j, char *key)
{
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();
  cee_map_remove(o, key);
}

void cee_json_object_set(struct cee_json *j, char *key, struct cee_json *v) {
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), v);
}

void cee_json_object_set_bool(struct cee_json * j, char * key, bool b) {
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_bool(b));
}

void cee_json_object_set_str (struct cee_json *j, char * key, char *str) {
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_str_mk(st, cee_str_mk(st, "%s", str)));
}

void cee_json_object_set_strf (struct cee_json *j, char * key, const char *fmt, ...) {
  va_list ap;
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();

  struct cee_state *st = cee_get_state(o);
  va_start(ap, fmt);
  struct cee_str *v = cee_str_mkv(st, fmt, ap);
  va_end(ap);

  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_str_mk(st, v));
}

void cee_json_object_set_double (struct cee_json *j, char *key, double real) {
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_double_mk(st, real));
}

void cee_json_object_set_i64 (struct cee_json *j, char *key, int64_t real) {
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_i64_mk(st, real));
}

void cee_json_object_set_u64 (struct cee_json * j, char * key, uint64_t real) {
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_u64_mk(st, real));
}

void cee_json_object_iterate (struct cee_json *j, void *ctx,
                              void (*f)(void *ctx, struct cee_str *key, struct cee_json *value))
{
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    cee_segfault();
  typedef void (*fnt)(void *, void*, void*);
  cee_map_iterate(o, ctx, (fnt)f);
};

void cee_json_array_append (struct cee_json * j, struct cee_json *v) {
  struct cee_list *o = cee_json_to_array(j);
  if (!o)
    cee_segfault();
  cee_list_append(&o, v);
  if (o != j->value.array) {

    j->value.array = o;
  }
}

void cee_json_array_append_bool (struct cee_json * j, bool b) {
  struct cee_list * o = cee_json_to_array(j);
  if (!o)
    cee_segfault();
  cee_list_append(&o, cee_json_bool(b));
  if (o != j->value.array) {

    j->value.array = o;
  }
}

void cee_json_array_append_str (struct cee_json * j, char * x) {
  struct cee_list *o = cee_json_to_array(j);
  if (!o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_list_append(&o, cee_json_str_mk(st, cee_str_mk(st, "%s", x)));
  if (o != j->value.array) {

    j->value.array = o;
  }
}

void cee_json_array_append_strf (struct cee_json *j, const char *fmt, ...) {
  va_list ap;
  struct cee_list *o = cee_json_to_array(j);
  if (!o)
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  va_start(ap, fmt);
  struct cee_str *v = cee_str_mkv(st, fmt, ap);
  va_end(ap);

  cee_list_append(&o, cee_json_str_mk(st, v));
  if (o != j->value.array) {

    j->value.array = o;
  }
}

struct cee_json* cee_json_array_get (struct cee_json *j, int i) {
  struct cee_list *o = cee_json_to_array(j);
  if (!o)
    cee_segfault();
  if (0 <= i && i < cee_list_size(o))
    return o->_[i];
  else
    return NULL;
}

void cee_json_array_remove(struct cee_json *j, int i) {
  struct cee_list *o = cee_json_to_array(j);
  if (!o)
    cee_segfault();
  if (0 <= i && i < cee_list_size(o))
    cee_list_remove(o, i);
}

void cee_json_array_iterate (struct cee_json *j, void *ctx,
        void (*f)(void *ctx, int index, struct cee_json *value))
{
  struct cee_list *o = cee_json_to_array(j);
  if (!o)
    cee_segfault();
  typedef void (*fnt)(void *, int, void*);
  cee_list_iterate(o, ctx, (fnt)f);
};




struct cee_json * cee_json_load_from_file (struct cee_state * st,
                                           FILE * f, bool force_eof,
                                           int * error_at_line) {
  int fd = fileno(f);
  struct stat buf;
  fstat(fd, &buf);
  off_t size = buf.st_size;
  char * b = malloc(size);
  if (!b)
    cee_segfault();

  int line = 0;
  struct cee_json * j;
  if (!cee_json_parse(st, b, size, &j, true, &line)) {

  }
  return j;
}

bool cee_json_save(struct cee_state * st, struct cee_json * j, FILE *f, int how) {
  size_t s = cee_json_snprint (st, NULL, 0, j, how);
  char * p = malloc(s+1);
  cee_json_snprint (st, p, s+1, j, how);
  if (fwrite(p, s+1, 1, f) != 1) {
    fprintf(stderr, "%s", strerror(errno));
    return false;
  }
  return true;
}
struct cee_json* cee_json_select(struct cee_json *o, char *fmt, ...) {
  enum next_selector_token {
    JSEL_INVALID = 0,
    JSEL_OBJ = 1,
    JSEL_ARRAY = 2,
    JSEL_TYPECHECK = 3,
    JSEL_MAX_TOKEN = 256
  } next = JSEL_INVALID;
  char token[JSEL_MAX_TOKEN+1];
  int tlen;
  va_list ap;

  va_start(ap,fmt);
  const char *p = fmt;
  tlen = 0;
  while(1) {


    if (tlen && (*p == '\0' || strchr(".[]:",*p))) {
      token[tlen] = '\0';
      if (next == JSEL_INVALID) {
 goto notfound;
      } else if (next == JSEL_ARRAY) {
 if (o->t != CEE_JSON_ARRAY) goto notfound;
 int idx = atoi(token);
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




      if (*p != '*') {
 token[tlen] = *p++;
 tlen++;
 if (tlen > JSEL_MAX_TOKEN) goto notfound;
 continue;
      } else {







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

 if (tlen+len > JSEL_MAX_TOKEN) goto notfound;
 memcpy(token+tlen,buf,len);
 tlen += len;
 p++;
 continue;
      }
    }

    if (*p == ']') p++;
    if (*p == '\0') break;
    else if (*p == '.') next = JSEL_OBJ;
    else if (*p == '[') next = JSEL_ARRAY;
    else if (*p == ':') next = JSEL_TYPECHECK;
    else goto notfound;
    tlen = 0;
    p++;
  }

cleanup:
  va_end(ap);
  return o;

notfound:
  o = NULL;
  goto cleanup;
}
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



bool cee_json_parse(struct cee_state * st, char * buf, uintptr_t len, struct cee_json **out, bool force_eof,
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
          top->_[1]=cee_json_array_mk(st, 10);
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
            top->_[1] = cee_json_i64_mk (st, tock.number.i64);
          else if (tock.type == NUMBER_IS_U64)
            top->_[1] = cee_json_u64_mk (st, tock.number.u64);
          else
            top->_[1] = cee_json_double_mk (st, tock.number.real);
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
          struct cee_json * a = cee_json_array_mk(st, 10);
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
          cee_list_append(&ar, cee_json_str_mk(st, tock.str));
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_true) {
          cee_list_append(&ar, cee_json_true());
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_false) {
          cee_list_append(&ar, cee_json_false());
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_null) {
          cee_list_append(&ar, cee_json_null());
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_number) {
          if (tock.type == NUMBER_IS_I64)
            cee_list_append(&ar, cee_json_double_mk(st, tock.number.i64));
          else if (tock.type == NUMBER_IS_U64)
            cee_list_append(&ar, cee_json_double_mk(st, tock.number.u64));
          else
            cee_list_append(&ar, cee_json_double_mk(st, tock.number.real));
          state=st_array_close_or_comma_expected;
        }
        else if(c=='[') {
          struct cee_json * a = cee_json_array_mk(st, 10);
          cee_list_append(&ar, a);
          state=st_array_value_or_close_expected;
          cee_stack_push(sp, cee_tuple_mk_e(st, (enum cee_del_policy [2]){CEE_DP_NOOP, CEE_DP_NOOP}, (void *)st_array_close_or_comma_expected, a));
        }
        else if(c=='{') {
          struct cee_json * o = cee_json_object_mk(st);
          cee_list_append(&ar, o);
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
        return false;
      }
    }
    *out = (struct cee_json *)(result->_[1]);
    cee_del(result);
    return true;
  }
  *error_at_line=tock.line;
  return false;
}
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
    p = cee_block_mk(st, sizeof(struct counter));
    p->tabs = 0;
  }
  else {
    switch(j->t) {
      case CEE_JSON_OBJECT:
        {
          p = cee_block_mk(st, sizeof(struct counter));
          struct cee_map * mp = cee_json_to_object(j);
          p->array = cee_map_keys(mp);
          p->object = cee_json_to_object(j);
          p->tabs = tabs;
          p->next = 0;
          p->more_siblings = 0;
        }
        break;
      case CEE_JSON_ARRAY:
        {
          p = cee_block_mk(st, sizeof(struct counter));
          p->array = cee_json_to_array(j);
          p->tabs = tabs;
          p->next = 0;
          p->more_siblings = 0;
        }
        break;
      default:
        {
          p = cee_block_mk(st, sizeof(struct counter));
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
    offset ++;
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
          if (cee_json_to_bool(cur_json))
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
            cee_boxed_snprint (buf+offset, incr+1 , cee_json_to_boxed(cur_json));
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
                  (struct cee_json *)(ccnt->array->_[i]));
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
            char * key = (char *)ccnt->array->_[i];
            struct cee_json * j1 = cee_map_find(ccnt->object, ccnt->array->_[i]);
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
    }
  }
  cee_del (sp);
  if (buf)
    buf[offset] = '\0';
  return offset;
}


ssize_t
cee_json_asprint(struct cee_state *st, char **buf_p, struct cee_json *j,
   enum cee_json_fmt f)
{
  size_t buf_size = cee_json_snprint(st, NULL, 0, j, f) + 1 ;
  char *buf = malloc(buf_size);
  *buf_p = buf;
  return cee_json_snprint(st, buf, buf_size, j, f);
}








static const uint32_t utf_illegal = 0xFFFFFFFFu;
static bool utf_valid(uint32_t v)
{
  if(v>0x10FFFF)
    return false;
  if(0xD800 <=v && v<= 0xDFFF)
    return false;
  return true;
}


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



static uint32_t next(char ** p, char * e, bool html)
{
  if(*p==e)
    return utf_illegal;

  unsigned char lead = **p;
  (*p)++;


  int trail_size = utf8_trail_length(lead);

  if(trail_size < 0)
    return utf_illegal;





  if(trail_size == 0) {
    if(!html || (lead >= 0x20 && lead!=0x7F) || lead==0x9 || lead==0x0A || lead==0x0D)
      return lead;
    return utf_illegal;
  }

  uint32_t c = lead & ((1<<(6-trail_size))-1);


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



  if(!utf_valid(c))
    return utf_illegal;


  if(utf8_width(c)!=trail_size + 1)
    return utf_illegal;

  if(html && c<0xA0)
    return utf_illegal;
  return c;
}
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
        break;
      }

      if (s == input_end) {

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
  char *start = t->buf + 1;
  char *end = start;


  while (*end != '\0' && *end != '"') {
    if ('\\' == *end++ && *end != '\0') {
      ++end;
    }
  }
  if (*end != '"') return false;

  char * unscp_str = NULL;
  size_t unscp_len = 0;
  if (json_string_unescape(&unscp_str, &unscp_len, start, end-start)) {
    if (unscp_str == start) {
      t->str = cee_str_mk_e(st, end-start+1, "%.*s", end-start, start);
    }
    else {

      t->str = cee_str_mk_e(st, unscp_len+1, "%s", unscp_str);
      free(unscp_str);
    }
    t->buf = end + 1;
    return true;
  }
  return false;
}

static bool parse_number(struct tokenizer *t) {
  char *start = t->buf;
  char *end = start;

  bool is_integer = true, is_exponent = false;
  int offset_sign = 0;


  if ('-' == *end) {
    ++end;
    offset_sign = 1;
  }
  if (!isdigit(*end))
    return false;


  while (isdigit(*++end))
    continue;


  if ('.' == *end) {
    if (!isdigit(*++end))
      return false;
    while (isdigit(*++end))
      continue;
    is_integer = false;
  }



  if ('e' == *end || 'E' == *end) {
    ++end;
    if ('+' == *end || '-' == *end)
      ++end;
    if (!isdigit(*end))
      return false;
    while (isdigit(*++end))
      continue;
    is_exponent = true;
  }
  else if (is_integer && (end-1) != (start+offset_sign) && '0' == start[offset_sign]) {
    return false;
  }


  char *endptr=NULL;
  if (is_exponent || !is_integer) {
    t->type = NUMBER_IS_DOUBLE;
    t->number.real = strtod(start, &endptr);
  }
  else {
    t->type = NUMBER_IS_I64;
    t->number.i64 = strtoll(start, &endptr, 10);
  }

  t->buf = end;

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

#endif
