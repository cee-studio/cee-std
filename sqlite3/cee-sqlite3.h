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

struct cee_sqlite3 {
  sqlite3 *db;
  struct cee_state *state;
};

extern sqlite3* cee_sqlite3_init_db(char *dbname, char *sqlstmts);

#define cee_sqlite3_begin_transaction(db)  sqlite3_exec(db, "begin transaction;", NULL, NULL, NULL)
#define cee_sqlite3_end_transaction(db)    sqlite3_exec(db, "end transaction;", NULL, NULL, NULL)

extern void cee_sqlite3_drop_all_tables(char *dbname);

enum cee_sqlite3_type {
  CEE_SQLITE3_INT,
  CEE_SQLITE3_INT64,
  CEE_SQLITE3_TEXT,
  CEE_SQLITE3_BLOB
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
  bool no_update; /* dont update this field if it's true, it's only effective for update */
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
  bool not_null;  /* the default is 0/false */
  bool no_return; /* dont return this colum, it's only effective for select, the default is 0/false */
};


struct cee_sqlite3_stmt_strs {
  char *table_name;
  char *select_stmt;
  char *insert_stmt; /* this has a higher precedence over insert_stmt_x */

  /* 
   * if dynamic insert is true, the following template is used to 
   *  compose insert_stmt_x:
   *
   * insert into table_name (%s) values (%s);
   */
  bool insert_dynamic;
  char *insert_stmt_x;
  
  char *delete_stmt;
  char *update_stmt; /* this has a higher precedence over update_stmt_x */

  /* 
   * a template of the following form is used to 
   *  compose update_stmt_x:
   *
   * update table set %s where ...;
   */
  char *update_template;
  char *update_stmt_x;

};

extern int
cee_sqlite3_bind_run_sql(struct cee_sqlite3 *cs,
                         struct cee_sqlite3_bind_info *info,
                         struct cee_sqlite3_bind_data *data,
                         char *sql, sqlite3_stmt **stmt_pp,
                         struct cee_json **ret);

extern int
cee_sqlite3_insert(struct cee_sqlite3 *cs,
                   struct cee_sqlite3_bind_info *info,
                   struct cee_sqlite3_bind_data *data,
                   struct cee_sqlite3_stmt_strs *stmts,
                   struct cee_json **status);

extern int
cee_sqlite3_delete(struct cee_sqlite3 *cs,
                   struct cee_sqlite3_bind_info *info,
                   struct cee_sqlite3_bind_data *data,
                   struct cee_sqlite3_stmt_strs *stmts,
                   struct cee_json **status);

extern int
cee_sqlite3_update(struct cee_sqlite3 *cs,
                   struct cee_sqlite3_bind_info *info,
                   struct cee_sqlite3_bind_data *data,
                   struct cee_sqlite3_stmt_strs *stmts,
                   struct cee_json **status);

extern int
cee_sqlite3_update_if_exists(struct cee_sqlite3 *cs,
                            struct cee_sqlite3_bind_info *info,
                            struct cee_sqlite3_bind_data *data,
                            struct cee_sqlite3_stmt_strs *stmts,
                            struct cee_json **status);

extern int
cee_sqlite3_update_or_insert(struct cee_sqlite3 *cs,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             struct cee_sqlite3_stmt_strs *stmts,
                             struct cee_json **status);

  
/*
 * the returned value is a json_array
 */
int 
cee_sqlite3_select(struct cee_sqlite3 *cs,
                   struct cee_sqlite3_bind_info *info,
                   struct cee_sqlite3_bind_data *data,
                   char *sql,
                   struct cee_json **status);

/*
 * status -> { key: list of selected items }
 * if key exists, the values are merged.
 */
int 
cee_sqlite3_select_as(struct cee_sqlite3 *cs,
		      struct cee_sqlite3_bind_info *info,
		      struct cee_sqlite3_bind_data *data,
		      char *sql,
		      struct cee_json **status,
		      char *key);

/*
 * the returned value is a json_object
 */
int 
cee_sqlite3_select1(struct cee_sqlite3 *cs,
		    struct cee_sqlite3_bind_info *info,
		    struct cee_sqlite3_bind_data *data,
		    char *sql,
		    struct cee_json **status);


/*
 * status -> { key: the selected value }
 */
int 
cee_sqlite3_select1_as(struct cee_sqlite3 *cs,
		       struct cee_sqlite3_bind_info *info,
		       struct cee_sqlite3_bind_data *data,
		       char *sql,
		       struct cee_json **status,
		       char *key);

/*
 * this is used to pass to generic_opcode function
 */
extern int
cee_sqlite3_select_wrapper(struct cee_sqlite3 *cs,
                           struct cee_sqlite3_bind_info *info,
                           struct cee_sqlite3_bind_data *data,
                           struct cee_sqlite3_stmt_strs *stmts,
                           struct cee_json **status);

/*
 * the returned value is a json_array if select succeeds,
 * or a json_object with last_insert_rowid set.
 */
extern int
cee_sqlite3_select_or_insert(struct cee_sqlite3 *cs,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             struct cee_sqlite3_stmt_strs *stmts,
                             struct cee_json **status);

/*
 * use JSON to bind sqlite3 stmts
 */
extern int
cee_sqlite3_generic_opcode(struct cee_sqlite3 *cs,
                           struct cee_json *json,
                           struct cee_sqlite3_bind_info *info,
                           struct cee_sqlite3_bind_data *data,
                           struct cee_sqlite3_stmt_strs *stmts,
                           struct cee_json **status,
                           int (*f)(struct cee_sqlite3 *cs,
                                    struct cee_sqlite3_bind_info *info,
                                    struct cee_sqlite3_bind_data *data,
                                    struct cee_sqlite3_stmt_strs *stmts,
                                    struct cee_json **status));


/*
 *  extract selected raw from status
 */
extern struct cee_json*
cee_sqlite3_get_selected(struct cee_json *status);

/*
 * return true if status contained at least one selected result
 */
extern bool
cee_sqlite3_has_selected_result(struct cee_json *status);

extern int
cee_sqlite3_get_pragma_variable(sqlite3 *db, char *name);


struct cee_sqlite3_db_op {
  struct cee_sqlite3_stmt_strs *stmts;
  struct cee_sqlite3_bind_info *info;
  struct cee_sqlite3_bind_data *data;  
};

extern int
cee_sqlite3_create_op(struct cee_sqlite3 *cs,
		      struct cee_sqlite3_db_op *op,
		      struct cee_json *input,
		      struct cee_json **status);

extern int
cee_sqlite3_update_or_create_op(struct cee_sqlite3 *cs,
				struct cee_sqlite3_db_op *op,
				struct cee_json *input,
				struct cee_json **status);

extern int
cee_sqlite3_update_op(struct cee_sqlite3 *cs,
		      struct cee_sqlite3_db_op *op,
		      struct cee_json *input,
		      struct cee_json **status);

extern int
cee_sqlite3_update_if_exists_op(struct cee_sqlite3 *cs,
				struct cee_sqlite3_db_op *op,
				struct cee_json *input,
				struct cee_json **status);

extern int
cee_sqlite3_read_op(struct cee_sqlite3 *cs,
		    struct cee_sqlite3_db_op *op,
		    struct cee_json *json,
		    struct cee_json **status);

extern int
cee_sqlite3_delete_op(struct cee_sqlite3 *cs,
		      struct cee_sqlite3_db_op *op,
		      struct cee_json *json);


extern struct cee_sqlite3_db_op*
new_cee_sqlite3_db_op(struct cee_state *state,
		      struct cee_sqlite3_db_op *op,
		      size_t size_of_bind_infos);
#endif
