#ifndef CEE_JSON_H
#define CEE_JSON_H
#ifndef CEE_JSON_AMALGAMATION
#include "cee.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#endif

#define MAX_JSON_DEPTH 500

struct cee_json_null {};
struct cee_json_undefined {};

enum cee_json_type {
  CEE_JSON_UNDEFINED,	///< Undefined value
  CEE_JSON_NULL,	///< null value
  CEE_JSON_BOOLEAN,	///< boolean value
  CEE_JSON_DOUBLE,	///< double value
  CEE_JSON_I64,         ///< 64-bit signed int
  CEE_JSON_U64,         ///< 65-bit unsigned int
  CEE_JSON_STRING,	///< string value
  CEE_JSON_OBJECT,	///< object value 
  CEE_JSON_ARRAY	///< array value
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

enum cee_json_format {
  cee_json_format_compact = 0,
  cee_json_format_readable = 1
};

extern enum cee_json_type cee_json_type  (struct cee_json *);
extern bool cee_json_is_undefined (struct cee_json *);
extern bool cee_json_is_null (struct cee_json *);
extern bool cee_json_to_bool (struct cee_state *, struct cee_json *);

extern struct json * cee_json_find (struct cee_json *, char *);
extern struct json * cee_json_get(struct cee_json *, char *, 
                                  struct cee_json * def);

extern bool cee_json_save (struct cee_state *, struct cee_json *, FILE *, int how);
extern struct cee_json * cee_json_load_from_file (struct cee_state *,
                                                  FILE *, bool force_eof, 
                                                  int * error_at_line);
extern struct cee_json * cee_json_load_from_buffer (int size, char *, int line);
extern int cee_json_cmp (struct cee_json *, struct cee_json *);

extern struct cee_list * cee_json_to_array (struct cee_json *);
extern struct cee_map * cee_json_to_object (struct cee_json *);
extern struct cee_boxed * cee_json_to_double (struct cee_json *);
extern struct cee_boxed * cee_json_to_i64 (struct cee_json*);
extern struct cee_boxed * cee_json_to_u64 (struct cee_json*);
extern struct cee_str * cee_json_to_string (struct cee_json *);

extern struct cee_json * cee_json_true (struct cee_state *);
extern struct cee_json * cee_json_false (struct cee_state *);
extern struct cee_json * cee_json_bool (struct cee_state *, bool b);
extern struct cee_json * cee_json_undefined (struct cee_state *);
extern struct cee_json * cee_json_null (struct cee_state *);
extern struct cee_json * cee_json_object_mk (struct cee_state *);
extern struct cee_json * cee_json_double_mk (struct cee_state *, double d);
extern struct cee_json * cee_json_i64_mk(struct cee_state *, int64_t);
extern struct cee_json * cee_json_u64_mk(struct cee_state *, uint64_t);
extern struct cee_json * cee_json_string_mk (struct cee_state *, struct cee_str * s);
extern struct cee_json * cee_json_array_mk (struct cee_state *, int s);

extern void cee_json_object_set (struct cee_state *, struct cee_json *, char *, struct cee_json *);
extern void cee_json_object_set_bool (struct cee_state *, struct cee_json *, char *, bool);
extern void cee_json_object_set_string (struct cee_state *, struct cee_json *, char *, char *);
extern void cee_json_object_set_double (struct cee_state *, struct cee_json *, char *, double);
extern void cee_json_object_set_i64 (struct cee_state *, struct cee_json *, char *, int64_t);
extern void cee_json_object_set_ui64 (struct cee_state *, struct cee_json *, char *, uint64_t);

extern void cee_json_array_append (struct cee_state *, struct cee_json *, struct cee_json *);
extern void cee_json_array_append_bool (struct cee_state *, struct cee_json *, bool);
extern void cee_json_array_append_string (struct cee_state *, struct cee_json *, char *);
extern void cee_json_array_append_double (struct cee_state *, struct cee_json *, double);
extern void cee_json_array_append_i64 (struct cee_state *, struct cee_json *, int64_t);
extern void cee_json_array_append_u64 (struct cee_state *, struct cee_json *, uint64_t);

extern size_t cee_json_snprint (struct cee_state *, char * buf,
                                size_t size, struct cee_json *, 
                                enum cee_json_format);

extern bool cee_json_parse(struct cee_state * st, char * buf, uintptr_t len, struct cee_json **out, 
                           bool force_eof, int *error_at_line);

#endif // CEE_JSON_H
