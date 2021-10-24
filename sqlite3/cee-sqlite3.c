#include "cee-sqlite3.h"

sqlite3* cee_sqlite3_init_db(char *dbname, char *sqlstmts)
{
  sqlite3_stmt *res;
  sqlite3 *db;

  int rc = sqlite3_open(dbname, &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    db = NULL;
    return NULL;
  }

  char *err_msg=NULL;
  rc = sqlite3_exec(db, "begin transaction;", 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return NULL;
  }

  rc = sqlite3_exec(db, sqlstmts, 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return NULL;
  }

  rc = sqlite3_exec(db, "commit;", 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return NULL;
  }
  return db;
}

void cee_sqlite3_drop_all_tables(char *dbname) {
  char *stmt = "PRAGMA writable_schema = 1;\n"
    "delete from sqlite_master where type in ('table', 'index', 'trigger');\n"
    "PRAGMA writable_schema = 0;\n";

  sqlite3 *db = cee_sqlite3_init_db(dbname, stmt);
  if (db)
    sqlite3_close(db);
}


int cee_sqlite3_bind_run_sql(struct cee_state *state,
                             sqlite3 *db,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             char *sql,
                             sqlite3_stmt **sql_stmt_pp,
                             struct cee_json **ret)
{
  sqlite3_stmt *sql_stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, 0);
  if (sql_stmt_pp)
    *sql_stmt_pp = sql_stmt;

  int idx = 0;
  struct cee_json *result = NULL;

  if (ret && *ret)
    result = *ret;

  if (rc == SQLITE_OK) {
    if (info) {
      for(int i = 0; info[i].var_name; i++) {
        idx = sqlite3_bind_parameter_index(sql_stmt, info[i].var_name);
        if (idx <= 0) continue;
	if (!data[i].has_value) continue;
        switch(info[i].type) 
        {
          case CEE_SQLITE3_INT:
            sqlite3_bind_int(sql_stmt, idx, data[i].i);
            break;
          case CEE_SQLITE3_INT64:
            sqlite3_bind_int64(sql_stmt, idx, data[i].i64);
            break;
          case CEE_SQLITE3_TEXT:
            sqlite3_bind_text(sql_stmt, idx, data[i].value, data[i].size == 0 ? -1: data[i].size, SQLITE_STATIC);
            break;
          case CEE_SQLITE3_BLOB:
            sqlite3_bind_blob(sql_stmt, idx, data[i].value, data[i].size, SQLITE_STATIC);
            break;
        }
      }
    }
    rc = sqlite3_step(sql_stmt);
    if (!sql_stmt_pp)
      sqlite3_finalize(sql_stmt);
    return rc;
  }
  else if (result)
    cee_json_object_set_strf(result, "error", "sqlite3:%s", sqlite3_errmsg(db));
  return rc;
}

struct cee_json*
cee_sqlite3_insert_or_update(struct cee_state *state,
                             sqlite3 *db,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             struct cee_sqlite3_stmts *stmts)
{
  sqlite3_stmt *sql_stmt;
  int step;
  struct cee_json *result = cee_json_object_mk(state);
  sqlite3_exec(db, "begin transaction;", NULL, NULL, NULL);
  step = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->select_stmt, NULL, &result);
  if (step == SQLITE_ROW) {
    step = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->update_stmt, NULL, &result);
    if (step != SQLITE_DONE)
      cee_json_object_set_strf(result, "error", "sqlite3:%s", sqlite3_errmsg(db));
  }
  else {
    step = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->insert_stmt, NULL, &result);
    if (step != SQLITE_DONE)
      cee_json_object_set_strf(result, "error", "sqlite3:%s", sqlite3_errmsg(db));
    else {
      int row_id = sqlite3_last_insert_rowid(db);
      cee_json_object_set_u64(result, "id", row_id);
    }
  }
  sqlite3_exec(db, "end transaction;", NULL, NULL, NULL);
  return result;
}


struct cee_json*
cee_sqlite3_update(struct cee_state *state,
                   sqlite3 *db,
                   struct cee_sqlite3_bind_info *info,
                   struct cee_sqlite3_bind_data *data,
                   struct cee_sqlite3_stmts *stmts)
{
  sqlite3_stmt *sql_stmt;
  int step;
  struct cee_json *result = cee_json_object_mk(state);
  sqlite3_exec(db, "begin transaction;", NULL, NULL, NULL);
  step = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->select_stmt, NULL, &result);
  if (step == SQLITE_ROW) {
    step = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->update_stmt, NULL, &result);
    if (step != SQLITE_DONE)
      cee_json_object_set_strf(result, "error", "sqlite3:%s", sqlite3_errmsg(db));
  }
  else
    cee_json_object_set_strf(result, "error", "sqlite3:'%s' returns no record", stmts->select_stmt);
  sqlite3_exec(db, "end transaction;", NULL, NULL, NULL);
  return result;
}


/*
 * returned value is a JSON array
 */
struct cee_json*
cee_sqlite3_select(struct cee_state *state,
		   sqlite3 *db,
		   struct cee_sqlite3_bind_info *info,
		   struct cee_sqlite3_bind_data *data,
		   char *sql)
{
  sqlite3_stmt *stmt = NULL;
  struct cee_json *array = cee_json_array_mk(state, 1);
  struct cee_json *status = NULL;
  int rc = cee_sqlite3_bind_run_sql(state, db, info, data, sql, &stmt, &status);
  while (rc != SQLITE_DONE) {
    struct cee_json *obj = cee_json_object_mk(state);
    cee_json_array_append(array, obj);
    
    int i, num_cols = sqlite3_column_count(stmt);
    for (i = 0; i < num_cols; i++) {
      char *name = (char *)sqlite3_column_name(stmt, i);
      switch(sqlite3_column_type(stmt, i)) {
      case SQLITE_TEXT:
	cee_json_object_set_str(obj, name, (char *)sqlite3_column_text(stmt, i));
	break;
      case SQLITE_INTEGER:
	cee_json_object_set_i64(obj, name, (double)sqlite3_column_int(stmt, i));
	break;
      default:
	fprintf(stderr, "uhandled name %s\n", name);
	break;
      }
    }
    rc = sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);
  return array;
}


struct cee_json*
cee_sqlite3_select_or_insert(struct cee_state *state,
                             sqlite3 *db,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             struct cee_sqlite3_stmts *stmts)
{
  sqlite3_stmt *sql_stmt;
  struct cee_json *array = cee_sqlite3_select(state, db, info, data, stmts->select_stmt);
  
  if (cee_json_select(array, "[0]"))
    return array;
  
  struct cee_json *result = cee_json_object_mk(state);
  sqlite3_exec(db, "begin transaction;", NULL, NULL, NULL);
  int step = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->insert_stmt, NULL, &result);
  if (cee_json_select(result, ".error"))
    goto clean_up;
  else if (step != SQLITE_DONE)
    cee_json_object_set_strf(result, "error", "sqlite3:%s", sqlite3_errmsg(db));
  else {
    int row_id = sqlite3_last_insert_rowid(db);
    cee_json_object_set_u64(result, "last_insert_rowid", row_id);
  }

 clean_up:
  sqlite3_exec(db, "end transaction;", NULL, NULL, NULL);
  return result;
}
