#ifndef CEE_JSON_AMALGAMATION
#include "json.h"
#include <stdlib.h>
#include "cee.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

struct cee_json * cee_json_true () {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *) cee_singleton_init ((uintptr_t)is_boolean, b);
}

struct cee_json * cee_json_false () {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *) cee_singleton_init ((uintptr_t)is_boolean, b);
}

struct cee_json * cee_json_bool(bool b) {
  if (b)
    return cee_json_true();
  else
    return cee_json_false();
}


struct cee_json * cee_json_undefined () {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init ((uintptr_t)is_undefined, b);
}

struct cee_json * cee_json_null () {
  static char b[CEE_SINGLETON_SIZE];
  return (struct cee_json *)cee_singleton_init ((uintptr_t)is_null, b);
}

struct cee_map * cee_json_to_object (struct cee_json * p) {
  if (p->t == is_object)
    return p->value.object;
  else
    return NULL;
}
struct cee_list * cee_json_to_array (struct cee_json * p) {
  if (p->t == is_array)
    return p->value.array;
  else
    return NULL;
}

struct cee_str * cee_json_to_string (struct cee_json * p) {
  if (p->t == is_string)
    return p->value.string;
  else
    return NULL;
}

struct cee_boxed * cee_json_to_number (struct cee_json * p) {
  if (p->t  == is_number)
    return p->value.number;
  else
    return NULL;
}

bool cee_json_to_bool (struct cee_json * p) {
  if (p == cee_json_true())
    return true;
  else if (p == cee_json_false())
    return false;
  
  cee_segfault();
}

struct cee_json * cee_json_number (double d) {
  struct cee_boxed * p = cee_boxed_from_double (d);
  struct cee_tagged * t = cee_tagged_mk (is_number, p);
  return (struct cee_json *)t;
}

struct cee_json * cee_json_string(struct cee_str *s) {
  struct cee_tagged * t = cee_tagged_mk (is_string, s);
  return (struct cee_json *)t;
}

struct cee_json * cee_json_array(int s) {
  struct cee_list * v = cee_list_mk (s);
  struct cee_tagged * t = cee_tagged_mk (is_array, v);
  return (struct cee_json *)t;
}

struct cee_json * cee_json_object() {
  struct cee_map * m = cee_map_mk ((cee_cmp_fun)strcmp);
  struct cee_tagged * t = cee_tagged_mk (is_object, m);
  return (struct cee_json *)t;
}

void cee_json_object_set(struct cee_json * j, char * key, struct cee_json * v) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk("%s", key), v);
}

void cee_json_object_set_bool(struct cee_json * j, char * key, bool b) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk("%s", key), cee_json_bool(b));
}

void cee_json_object_set_string (struct cee_json * j, char * key, char * str) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk("%s", key), cee_json_string(cee_str("%s", str)));
}

void cee_json_object_set_number (struct cee_json * j, char * key, double real) {
  struct cee_map * o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  cee_map_add(o, cee_str_mk("%s", key), cee_json_number(real));
}

void cee_json_array_append (struct cee_json * j, struct cee_json *v) {
  struct cee_list * o = cee_json_to_array(j);
  if (!o) 
    cee_segfault();
  cee_list_append(&o, v);
}

void cee_json_array_append_bool (struct cee_json * j, bool b) {
  struct cee_list * o = cee_json_to_array(j);
  if (!o) 
    cee_segfault();
  cee_list_append(&o, cee_json_bool(b));
}

void cee_json_array_append_string (struct cee_json * j, char * x) {
  struct cee_list * o = cee_json_to_array(j);
  if (!o) 
    cee_segfault();
  cee_list_append(&o, cee_json_string(cee_str_mk("%s", x)));
}

/*
 * this function assume the file pointer points to the begin of a file
 */
struct cee_json * cee_json_load_from_file (FILE * f, bool force_eof, 
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
  if (!cee_json_parse(b, size, &j, true, &line)) {
    // report error
  }
  return j;
}

bool cee_json_save(struct cee_json * j, FILE *f, int how) {
  size_t s = cee_json_snprint (NULL, 0, j, how);
  char * p = malloc(s+1);
  cee_json_snprint (p, s+1, j, how);
  if (fwrite(p, s+1, 1, f) != 1) {
    fprintf(stderr, "%s", strerror(errno));
    return false;
  }
  return true;
}
