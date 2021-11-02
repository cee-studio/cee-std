#ifndef CEE_JSON_AMALGAMATION
#include "cee-json.h"
#include <stdlib.h>
#include "cee.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#endif


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


bool cee_json_to_double (struct cee_json *p, double *r) {
  if (p->t == CEE_JSON_DOUBLE) {
    *r = cee_boxed_to_double(p->value.boxed);
    return true;
  }
  else
    return false;
}

bool cee_json_to_i64 (struct cee_json *p, int64_t *r) {
  if (p->t == CEE_JSON_I64) {
    *r = cee_boxed_to_i64(p->value.boxed);
    return true;
  }
  else
    return false;
}

bool cee_json_to_u64 (struct cee_json *p, uint64_t *r) {
  if (p->t == CEE_JSON_U64) {
    *r = cee_boxed_to_u64(p->value.boxed);
    return true;
  }
  else
    return false;
}

bool cee_json_to_bool(struct cee_json *p, bool *r) {
  if (p == cee_json_true()) {
    *r = true;
    return true;
  }
  else if (p == cee_json_false()) {
    *r = false;
    return true;
  }
  else
    return false;
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
  struct cee_block *m = cee_block_mk(st, bytes);
  memcpy(m->_, src, bytes);
  struct cee_tagged *t = cee_tagged_mk (st, CEE_JSON_BLOB, m);
  return (struct cee_json*)t;
}

bool cee_json_object_replace(struct cee_json *j, char *old_key, char *new_key) {
  struct cee_map *o = cee_json_to_object(j);
  if (!o)
    return false;
  struct cee_state *st = cee_get_state(o);
  struct cee_str *old_str = cee_str_mk(st, "%s", old_key);
  struct cee_str *new_str = cee_str_mk(st, "%s", new_key);
  bool t = cee_map_replace(o, old_str, new_str);
  cee_del(old_str);
  return t;
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

void cee_json_object_set_error(struct cee_json *o, const char *fmt, ...) {
  if (!o) return;
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


void cee_json_object_set(struct cee_json *j, char *key, struct cee_json *v) {
  if (!j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), v);
}

void cee_json_object_set_bool(struct cee_json *j, char *key, bool b) {
  if (!j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_bool(b));
}

void cee_json_object_set_str (struct cee_json *j, char *key, char *str) {
  if (!j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_str_mk(st, cee_str_mk(st, "%s", str)));
}

void cee_json_object_set_strf (struct cee_json *j, char *key, const char *fmt, ...) {
  if (!j) return;
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
  if (!j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_double_mk(st, real));
}

void cee_json_object_set_i64 (struct cee_json *j, char *key, int64_t real) {
  if (!j) return;
  struct cee_map *o = cee_json_to_object(j);
  if (!o) 
    cee_segfault();
  struct cee_state *st = cee_get_state(o);
  cee_map_add(o, cee_str_mk(st, "%s", key), cee_json_i64_mk(st, real));
}

void cee_json_object_set_u64 (struct cee_json *j, char *key, uint64_t real) {
  if (!j) return;
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
    /*  free j->value.array */
    j->value.array = o;
  }
}

void cee_json_array_append_bool (struct cee_json * j, bool b) {
  struct cee_list * o = cee_json_to_array(j);
  if (!o) 
    cee_segfault();
  cee_list_append(&o, cee_json_bool(b));
  if (o != j->value.array) {
    /*  free j->value.array */
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
    /*  free j->value.array */
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
    /*  free j->value.array */
    j->value.array = o;
  }
}

size_t cee_json_array_length(struct cee_json *j) {
  struct cee_list *o = cee_json_to_array(j);
  if (!o)
    cee_segfault();
  return cee_list_size(o);
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
    /*  report error */
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
  enum next_selector_token {
    JSEL_INVALID = 0,
    JSEL_OBJ = 1,            /* "." */
    JSEL_ARRAY = 2,          /* "[" */
    JSEL_TYPECHECK = 3,      /* ":" */
    JSEL_MAX_TOKEN = 256
  } next = JSEL_INVALID;          /* Type of the next selector. */
  char token[JSEL_MAX_TOKEN+1];   /* Current token. */
  int tlen;                       /* Current length of the token. */
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
