#ifndef CEE_JSON_AMALGAMATION
#include "cee-json.h"
#include <stdlib.h>
#include "cee.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

struct cee_json * cee_json_true (struct cee_state * st) {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init(b, (uintptr_t)CEE_JSON_BOOLEAN, 1);
}

struct cee_json * cee_json_false (struct cee_state * st) {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init(b, (uintptr_t)CEE_JSON_BOOLEAN, 0);
}

struct cee_json * cee_json_bool (struct cee_state * st, bool b) {
  return b ? cee_json_true(st) : cee_json_false(st);
}


struct cee_json * cee_json_undefined (struct cee_state * st) {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init(b, (uintptr_t)CEE_JSON_UNDEFINED, 0);
}

struct cee_json * cee_json_null (struct cee_state * st) {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init(b, (uintptr_t)CEE_JSON_UNDEFINED, 0);
}

struct cee_map * cee_json_to_object (struct cee_json * p) {
  return (p->t == CEE_JSON_OBJECT) ? p->value.object : NULL;
}
struct cee_list * cee_json_to_array (struct cee_json * p) {
  return (p->t == CEE_JSON_ARRAY) ? p->value.array : NULL;
}
struct cee_str * cee_json_to_string (struct cee_json * p) {
  return (p->t == CEE_JSON_STRING) ? p->value.string : NULL;
}
struct cee_boxed * cee_json_to_double (struct cee_json * p) {
  return (p->t == CEE_JSON_DOUBLE) ? p->value.boxed : NULL;
}

struct cee_boxed * cee_json_to_i64 (struct cee_json * p) {
  return (p->t == CEE_JSON_I64) ? p->value.boxed : NULL;
}

struct cee_boxed * cee_json_to_u64 (struct cee_json * p) {
  return (p->t == CEE_JSON_U64) ? p->value.boxed : NULL;
}

bool cee_json_to_bool (struct cee_state * st, struct cee_json * p) {
  if (p == cee_json_true(st))
    return true;
  else if (p == cee_json_false(st))
    return false;
  cee_segfault();
}

struct cee_json * cee_json_double_mk (struct cee_state * st, double d) {
  struct cee_boxed * p = cee_boxed_from_double (st, d);
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_DOUBLE, p);
}

struct cee_json * cee_json_i64_mk (struct cee_state * st, int64_t d) {
  struct cee_boxed * p = cee_boxed_from_i64 (st, d);
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_I64, p);
}

struct cee_json * cee_json_u64_mk (struct cee_state * st, uint64_t d) {
  struct cee_boxed * p = cee_boxed_from_u64 (st, d);
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_U64, p);
}

struct cee_json * cee_json_string_mk(struct cee_state * st, struct cee_str *s) {
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_STRING, s);
}

struct cee_json * cee_json_array_mk(struct cee_state * st, int s) {
  struct cee_list * v = cee_list_mk (st, s);
  return (struct cee_json *)cee_tagged_mk (st, CEE_JSON_ARRAY, v);
}

struct cee_json * cee_json_object_mk(struct cee_state * st) {
  struct cee_map * m = cee_map_mk (st, (cee_cmp_fun)strcmp);
  struct cee_tagged * t = cee_tagged_mk (st, CEE_JSON_OBJECT, m);
  return (struct cee_json *)t;
}

void cee_json_object_set(struct cee_state * st, struct cee_json * j, char * key, struct cee_json * v) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk(st, "%s", key), v);
}

void cee_json_object_set_bool(struct cee_state * st, struct cee_json * j, char * key, bool b) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_bool(st, b));
}

void cee_json_object_set_string (struct cee_state * st, struct cee_json * j, char * key, char * str) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_string_mk(st, cee_str_mk(st, "%s", str)));
}

void cee_json_object_set_double (struct cee_state * st, struct cee_json * j, char * key, double real) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_double_mk(st, real));
}

void cee_json_object_set_i64 (struct cee_state * st, struct cee_json * j, char * key, int64_t real) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_i64_mk(st, real));
}

void cee_json_object_set_u64 (struct cee_state * st, struct cee_json * j, char * key, uint64_t real) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_u64_mk(st, real));
}

void cee_json_array_append (struct cee_state * st, struct cee_json * j, struct cee_json *v) {
  (void)st;
  struct cee_list * o = cee_json_to_array(j);
  if (!o) 
    cee_segfault();
  cee_list_append(&o, v);
  if (o != j->value.array) {
    // free j->value.array
    j->value.array = o;
  }
}

void cee_json_array_append_bool (struct cee_state * st, struct cee_json * j, bool b) {
  struct cee_list * o = cee_json_to_array(j);
  if (!o) 
    cee_segfault();
  cee_list_append(&o, cee_json_bool(st, b));
  if (o != j->value.array) {
    // free j->value.array
    j->value.array = o;
  }
}

void cee_json_array_append_string (struct cee_state * st, struct cee_json * j, char * x) {
  struct cee_list * o = cee_json_to_array(j);
  if (!o) 
    cee_segfault();
  cee_list_append(&o, cee_json_string_mk(st, cee_str_mk(st, "%s", x)));
  if (o != j->value.array) {
    // free j->value.array
    j->value.array = o;
  }
}

struct cee_json* cee_json_array_get (struct cee_state *st, struct cee_json *j, int i) {
  struct cee_list *o = cee_json_to_array(j);
  if (!o)
    cee_segfault();
  if (0 <= i && i < cee_list_size(o))
    return o->_[i];
  else
    return NULL;
}

/*
 * this function assume the file pointer points to the begin of a file
 */
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
    // report error
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
