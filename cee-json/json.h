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
  is_undefined,	///< Undefined value
  is_null,	///< null value
  is_boolean,	///< boolean value
  is_number,	///< numeric value
  is_string,	///< string value
  is_object,	///< object value 
  is_array	///< array value
};

struct cee_json {
	enum cee_json_type t;
  union {
    struct cee_singleton * null;
    struct cee_singleton * undefined;
    struct cee_singleton * boolean;
    struct cee_boxed     * number;
    struct cee_str       * string;
    struct cee_list      * array;
    struct cee_map       * object;
  } value;
};

enum cee_json_format {
  compact = 0,
  readable = 1
};

extern enum cee_json_type cee_json_type  (struct cee_json *);
extern bool cee_json_is_undefined (struct cee_json *);
extern bool cee_json_is_null (struct cee_json *);
extern bool cee_json_to_bool (struct cee_json *);

extern struct json * cee_json_find (struct cee_json *, char *);
extern struct json * cee_json_get(struct cee_json *, char *, 
                                  struct cee_json * def);

extern bool cee_json_save (struct cee_json *, FILE *, int how);
extern struct cee_json * cee_json_load_from_file (FILE *, bool force_eof, 
                                                  int * error_at_line);
extern struct cee_json * cee_json_load_from_buffer (int size, char *, int line);
extern int cee_json_cmp (struct cee_json *, struct cee_json *);

extern struct cee_list * cee_json_to_array (struct cee_json *);
extern struct cee_map * cee_json_to_object (struct cee_json *);
extern struct cee_boxed * cee_json_to_number (struct cee_json *);
extern struct cee_str * cee_json_to_string (struct cee_json *);

extern struct cee_json * cee_json_true();
extern struct cee_json * cee_json_false();
extern struct cee_json * cee_json_undefined ();
extern struct cee_json * cee_json_null ();
extern struct cee_json * cee_json_object();
extern struct cee_json * cee_json_number (double d);
extern struct cee_json * cee_json_string(struct cee_str * s);
extern struct cee_json * cee_json_array(int s);

extern void cee_json_object_set (struct cee_json *, char *, struct cee_json *);
extern void cee_json_object_set_bool (struct cee_json *, char *, bool);
extern void cee_json_object_set_string (struct cee_json *, char *, char *);
extern void cee_json_object_set_number (struct cee_json *, char *, double);

extern void cee_json_array_append (struct cee_json *, struct cee_json *);
extern void cee_json_array_append_bool (struct cee_json *, bool);
extern void cee_json_array_append_string (struct cee_json *, char *);
extern void cee_json_array_append_number (struct cee_json *, double);

extern size_t cee_json_snprint (char * buf, size_t size, struct cee_json *, 
                                enum cee_json_format);

extern bool cee_json_parse(char * buf, uintptr_t len, struct cee_json **out, 
                           bool force_eof, int *error_at_line);

#endif // ORCA_JSON_H