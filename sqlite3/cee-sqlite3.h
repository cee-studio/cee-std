#ifndef CEE_SQLITE3_H
#define CEE_SQLITE3_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "sqlite3.h"
#include "cee-json.h"

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

struct cee_sqlite3_bind_info {
  char *var_name; /* name for the binding variable */
  char *col_name; /* db column name */
  char *ext_name; /* external name */
  enum cee_sqlite3_type type;
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

struct cee_sqlite3_stmts {
  char *select_stmt;
  char *update_stmt;
  char *insert_stmt;
  char *delete_stmt;
};

extern int
cee_sqlite3_bind_run_sql(struct cee_state *state,
                         sqlite3 *db,
                         struct cee_sqlite3_bind_info *info,
                         struct cee_sqlite3_bind_data *data,
                         char *sql, sqlite3_stmt **stmt_pp,
                         struct cee_json **ret);

extern struct cee_json*
cee_sqlite3_insert_or_update(struct cee_state *state,
                             sqlite3 *db,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             struct cee_sqlite3_stmts *stmts);

extern struct cee_json*
cee_sqlite3_update(struct cee_state *state,
                   sqlite3 *db,
                   struct cee_sqlite3_bind_info *info,
                   struct cee_sqlite3_bind_data *data,
                   struct cee_sqlite3_stmts *stmts);


/*
 * the returned value is a json_array
 */
extern struct cee_json*
cee_sqlite3_select(struct cee_state *state,
		   sqlite3 *db,
		   struct cee_sqlite3_bind_info *info,
		   struct cee_sqlite3_bind_data *data,
		   char *sql);

/*
 * the returned value is a json_array if select succeeds,
 * or a json_object with last_insert_rowid set.
 */
extern struct cee_json*
cee_sqlite3_select_or_insert(struct cee_state *state,
                             sqlite3 *db,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             struct cee_sqlite3_stmts *stmts);

#endif
