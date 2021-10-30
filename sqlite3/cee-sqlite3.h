#ifndef CEE_SQLITE3_H
#define CEE_SQLITE3_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "sqlite3.h"
#include "cee-json.h"

/*
 * a simple SQL Data <-> JSON binding
 */

extern sqlite3* cee_sqlite3_init_db(char *dbname, char *sqlstmts);
extern void cee_sqlite3_drop_all_tables(char *dbname);

enum cee_sqlite3_type {
  CEE_SQLITE3_INT,
  CEE_SQLITE3_INT64,
  CEE_SQLITE3_TEXT,
  CEE_SQLITE3_BLOB
};
  
struct cee_sqlite3_bind_info_data {
  char *name; /* name for the binding variable */
  enum cee_sqlite3_type type;
  void *value;
};

/*
 * the value of binding data
 */
struct cee_sqlite3_bind_data {
  int i;
  int64_t i64;
  char *value;
  size_t size;
  int has_value;
};

struct cee_sqlite3_bind_info {
  char *var_name; /* name for the binding variable */
  char *col_name; /* db column name */
  char *ext_name; /* external name */
  enum cee_sqlite3_type type;
  struct cee_sqlite3_bind_data data; /* default data is used if external data is not provided */
  bool no_update; /* dont update this field if it's true */
  /* 
   * this field will only affect insert
   *
   * this field cannot be null. if it's true
   * the default value will be used if 
   * data.has_value is false.
   *
   * default values:
   *
   * int: 0
   * text: ""
   * blob: size 0
   */
  bool not_null;  
};


struct cee_sqlite3_stmt_strs {
  char *table_name;
  char *select_stmt;
  char *insert_stmt;
  char *delete_stmt;
  char *update_stmt; /* this has a higher precedence over update_stmt_x */

  /* 
   * a template of the following form is used to 
   *  compose stmt_x:
   *
   * update table set %s where ...;
   */
  char *update_template;
  char *update_stmt_x;

};

extern int
cee_sqlite3_bind_run_sql(struct cee_state *state,
                         sqlite3 *db,
                         struct cee_sqlite3_bind_info *info,
                         struct cee_sqlite3_bind_data *data,
                         char *sql, sqlite3_stmt **stmt_pp,
                         struct cee_json **ret);

extern struct cee_json*
cee_sqlite3_insert(struct cee_state *state,
		   sqlite3 *db,
		   struct cee_sqlite3_bind_info *info,
		   struct cee_sqlite3_bind_data *data,
		   struct cee_sqlite3_stmt_strs *stmts);

extern struct cee_json*
cee_sqlite3_update(struct cee_state *state,
                   sqlite3 *db,
                   struct cee_sqlite3_bind_info *info,
                   struct cee_sqlite3_bind_data *data,
                   struct cee_sqlite3_stmt_strs *stmts);

extern struct cee_json*
cee_sqlite3_update_or_insert(struct cee_state *state,
                             sqlite3 *db,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             struct cee_sqlite3_stmt_strs *stmts);

  
/*
 * the returned value is a json_array
 */
struct cee_json*
cee_sqlite3_select(struct cee_state *state,
		   sqlite3 *db,
		   struct cee_sqlite3_bind_info *info,
		   struct cee_sqlite3_bind_data *data,
		   char *sql);

/*
 * this is used to pass to generic_opcode function
 */
extern struct cee_json*
cee_sqlite3_select_wrapper(struct cee_state *state,
			   sqlite3 *db,
			   struct cee_sqlite3_bind_info *info,
			   struct cee_sqlite3_bind_data *data,
			   struct cee_sqlite3_stmt_strs *stmts);

/*
 * the returned value is a json_array if select succeeds,
 * or a json_object with last_insert_rowid set.
 */
extern struct cee_json*
cee_sqlite3_select_or_insert(struct cee_state *state,
                             sqlite3 *db,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             struct cee_sqlite3_stmt_strs *stmts);

/*
 * use JSON to bind sqlite3 stmts
 */
extern struct cee_json*
cee_sqlite3_generic_opcode(struct cee_state *st,
			   sqlite3 *db,
			   struct cee_json *json,
			   struct cee_sqlite3_bind_info *info,
			   struct cee_sqlite3_bind_data *data,
			   struct cee_sqlite3_stmt_strs *stmts,
			   struct cee_json* (*f)(struct cee_state *st,
						 sqlite3 *db,
						 struct cee_sqlite3_bind_info *info,
						 struct cee_sqlite3_bind_data *data,
						 struct cee_sqlite3_stmt_strs *stmts));
#endif
