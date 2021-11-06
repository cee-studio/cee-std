#include "cee-sqlite3.h"
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>

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

  if (!sqlstmts)
    return db;

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

enum stmt_type {
  SELECT = 0,
  INSERT,
  UPDATE,
  DELETE
};

static enum stmt_type get_stmt_type(const char *stmt) {
  const char *p = stmt;
  while (isspace(*p)) p++;
  char *loc; ;
  if ((loc = strcasestr(p, "select")) == p)
    return SELECT;
  else if ((loc = strcasestr(p, "insert")) == p)
    return INSERT;
  else if ((loc = strcasestr(p, "update")) == p)
    return UPDATE;
  else if ((loc = strcasestr(p, "delete")) == p)
    return DELETE;
  cee_segfault();
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
  enum stmt_type stmt_type = get_stmt_type(sql);

  int rc = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, 0);
  if (sql_stmt_pp)
    *sql_stmt_pp = sql_stmt;

  int idx = 0;
  struct cee_json *result = NULL;
  struct cee_sqlite3_bind_data *data_p = NULL;

  if (ret) {
    if (*ret)
      result = *ret;
    else {
      result = cee_json_object_mk(state);
      *ret = result;
    }
  }

  if (rc == SQLITE_OK) {
    if (info) {
      for(int i = 0; info[i].var_name; i++) {
        idx = sqlite3_bind_parameter_index(sql_stmt, info[i].var_name);
        if (idx <= 0)
          continue;

        if (data && data[i].has_value)
          data_p = data+i;
        else if (info[i].data.has_value)
          data_p = &(info[i].data);
        else if (stmt_type == INSERT && info[i].not_null) {
          switch(info[i].type)
            {
            case CEE_SQLITE3_INT:
              sqlite3_bind_int(sql_stmt, idx, 0);
              break;
            case CEE_SQLITE3_INT64:
              sqlite3_bind_int64(sql_stmt, idx, 0);
              break;
            case CEE_SQLITE3_TEXT:
              sqlite3_bind_text(sql_stmt, idx, "", 0, SQLITE_STATIC);
              break;
            case CEE_SQLITE3_BLOB:
              sqlite3_bind_blob(sql_stmt, idx, "", 0, SQLITE_STATIC);
              break;
            default:
              cee_segfault();
              break;
            }
          continue;
        }
        else
          continue;

        switch(info[i].type) 
          {
          case CEE_SQLITE3_INT:
            sqlite3_bind_int(sql_stmt, idx, data_p->i);
            break;
          case CEE_SQLITE3_INT64:
            sqlite3_bind_int64(sql_stmt, idx, data_p->i64);
            break;
          case CEE_SQLITE3_TEXT:
            sqlite3_bind_text(sql_stmt, idx, data_p->value, 
                              data_p->size == 0 ? -1: data_p->size, SQLITE_STATIC);
            break;
          case CEE_SQLITE3_BLOB:
            sqlite3_bind_blob(sql_stmt, idx, data_p->value, 
                              data_p->size, SQLITE_STATIC);
            break;
          default:
            cee_segfault();
            break;
          }
      }
    }
    rc = sqlite3_step(sql_stmt);
    if (!sql_stmt_pp)
      sqlite3_finalize(sql_stmt);
    return rc;
  }
  else
    cee_json_set_error(ret, "sqlite3:[%d]'%s' -> %s", rc, sql, sqlite3_errmsg(db));
  return rc;
}


static void accept_result(struct cee_state *state,
                          struct cee_json **status, struct cee_json **result_p) {
  if (status) {
    /* the caller wants to receive executions status and result */
    if (*status == NULL) {
      /* the caller wants callees to allocate memory */
      *result_p = cee_json_object_mk(state);
      *status = *result_p;
    }
    else
      *result_p = *status;
  }
  else
    *result_p = NULL;
}

int cee_sqlite3_update_or_insert(struct cee_state *state,
                                 sqlite3 *db,
				 struct cee_sqlite3_bind_info *info,
                                 struct cee_sqlite3_bind_data *data,
                                 struct cee_sqlite3_stmt_strs *stmts,
                                 struct cee_json **status)
{
  int rc;
  struct cee_json *result = NULL;
  accept_result(state, status, &result);
  
  rc = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->select_stmt, NULL, status);
  if (rc == SQLITE_ROW) {
    char *update = stmts->update_stmt ? stmts->update_stmt : stmts->update_stmt_x;
    if (!update) cee_segfault();
    rc = cee_sqlite3_bind_run_sql(state, db, info, data, update, NULL, status);
    if (rc != SQLITE_DONE)
      cee_json_set_error(status, "sqlite3:[%d]'%s' -> %s", rc, update, sqlite3_errmsg(db));
  }
  else {
    rc = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->insert_stmt, NULL, status);
    if (rc != SQLITE_DONE)
      cee_json_set_error(status, "sqlite3:[%d]'%s' -> %s",
             rc, stmts->insert_stmt, sqlite3_errmsg(db));
    else if (result) {
      int row_id = sqlite3_last_insert_rowid(db);
      cee_json_object_set_i64(result, "last_insert_rowid", row_id);
    }
  }
  return rc;
}

int cee_sqlite3_insert(struct cee_state *state,
                       sqlite3 *db,
                       struct cee_sqlite3_bind_info *info,
                       struct cee_sqlite3_bind_data *data,
                       struct cee_sqlite3_stmt_strs *stmts,
                       struct cee_json **status)
{
  sqlite3_stmt *sql_stmt;
  int rc;
  struct cee_json *result = NULL;

  accept_result(state, status, &result);
  rc = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->insert_stmt, NULL, status);
  if (rc != SQLITE_DONE)
    cee_json_set_error(status, "sqlite3:[%d]'%s' -> %s", rc, stmts->insert_stmt, sqlite3_errmsg(db));
  else if (result) {
    int row_id = sqlite3_last_insert_rowid(db);
    cee_json_object_set_i64(result, "last_insert_rowid", row_id);
  }
  return rc;
}


int cee_sqlite3_update(struct cee_state *state,
               sqlite3 *db,
               struct cee_sqlite3_bind_info *info,
               struct cee_sqlite3_bind_data *data,
               struct cee_sqlite3_stmt_strs *stmts,
               struct cee_json **status)
{
  sqlite3_stmt *sql_stmt;
  int rc;
  struct cee_json *result = NULL;

  accept_result(state, status, &result);

  rc = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->select_stmt, NULL, status);
  if (rc == SQLITE_ROW) {
    char *update = stmts->update_stmt ? stmts->update_stmt : stmts->update_stmt_x;
    if (!update) cee_segfault();
    rc = cee_sqlite3_bind_run_sql(state, db, info, data, update, NULL, status);
    if (rc != SQLITE_DONE)
      cee_json_set_error(status, "sqlite3:[%d]'%s' -> %s", rc, update, sqlite3_errmsg(db));
  }
  else
    cee_json_set_error(status, "sqlite3:[%d]'%s' -> %s", rc, sqlite3_errmsg(db));
  return rc;
}


/*
 * returned value is a JSON array
 */
int cee_sqlite3_select(struct cee_state *state,
                       sqlite3 *db,
                       struct cee_sqlite3_bind_info *info,
                       struct cee_sqlite3_bind_data *data,
                       char *sql,
                       struct cee_json **status)
{
  sqlite3_stmt *stmt = NULL;
  struct cee_json *result = NULL, *array = cee_json_array_mk(state, 1);
  accept_result(state, status, &result);
  
  int rc = cee_sqlite3_bind_run_sql(state, db, info, data, sql, &stmt, status);
  if (rc != SQLITE_ROW && rc != DONE) {
    cee_json_set_error(status, "sqlite3:[%d]'%s' %s", rc, sql, sqlite3_errmsg(db));
    return rc;
  }
  
  while (rc != SQLITE_DONE) {
    struct cee_json *obj = cee_json_object_mk(state);
    cee_json_array_append(array, obj);
    
    int i, num_cols = sqlite3_column_count(stmt);
    for (i = 0; i < num_cols; i++) {
      char *name = (char *)sqlite3_column_name(stmt, i);
      switch(sqlite3_column_type(stmt, i)) {
      case SQLITE_INTEGER:
        cee_json_object_set_i64(obj, name, sqlite3_column_int64(stmt, i));
        break;
      case SQLITE_FLOAT:
        cee_json_object_set_double(obj, name, sqlite3_column_double(stmt, i));
        break;
      case SQLITE_TEXT:
        cee_json_object_set_str(obj, name, (char*)sqlite3_column_text(stmt, i));
        break;
      case SQLITE_NULL:
        cee_json_object_set(obj, name, cee_json_null());
        break;
      case SQLITE_BLOB: {
          size_t bytes = sqlite3_column_bytes(stmt, i);
          const void *data = sqlite3_column_blob(stmt, i);
          struct cee_json *blob = cee_json_blob_mk (state, data, bytes);
          cee_json_object_set(obj, name, blob);
        }
        break;
      }
    }
    rc = sqlite3_step(stmt);
  }
  sqlite3_finalize(stmt);
  if (result)
    cee_json_object_set(result, "sqlite3_selected", array);

  return rc;
}


/*
 * returned value is a JSON array
 */
int cee_sqlite3_select_wrapper(struct cee_state *state,
                   sqlite3 *db,
                   struct cee_sqlite3_bind_info *info,
                   struct cee_sqlite3_bind_data *data,
                   struct cee_sqlite3_stmt_strs *stmts,
                   struct cee_json **status)
{
  return cee_sqlite3_select(state, db, info, data, stmts->select_stmt, status);
}

int cee_sqlite3_select_or_insert(struct cee_state *state,
                 sqlite3 *db,
                 struct cee_sqlite3_bind_info *info,
                 struct cee_sqlite3_bind_data *data,
                 struct cee_sqlite3_stmt_strs *stmts,
                 struct cee_json **status)
{
  sqlite3_stmt *sql_stmt;
  struct cee_json *result = NULL;
  accept_result(state, status, &result);
  
  int rc = cee_sqlite3_select(state, db, info, data, stmts->select_stmt, status);

  if (result && cee_json_select(result, ".sqlite3_selected[0]"))
    return rc;

  rc = cee_sqlite3_bind_run_sql(state, db, info, data, stmts->insert_stmt, NULL, status);

  if (rc != SQLITE_DONE)
    cee_json_set_error(status, "sqlite3:[%d]'%s' -> %s",
               rc, stmts->insert_stmt, sqlite3_errmsg(db));
  else if (result) {
    int row_id = sqlite3_last_insert_rowid(db);
    cee_json_object_set_i64(*status, "last_insert_rowid", row_id);
  }

  return rc;
}


/*
 * a priviate struct
 */
struct _combo {
  struct cee_state *state;
  struct cee_json *json_in;
  struct cee_json *errors;
  struct cee_json *unused;
  struct cee_json *used;
  struct cee_sqlite3_stmt_strs *stmts;
  struct cee_sqlite3_bind_info *info;
  struct cee_sqlite3_bind_data *data;
  struct cee_str *update_set;
};

static void
populate_opcode(void *ctx, struct cee_str *key, struct cee_json *value) {
  struct _combo *p = ctx;
  struct cee_sqlite3_stmt_strs *stmts = p->stmts;
  struct cee_sqlite3_bind_info *info = p->info;
  struct cee_sqlite3_bind_data *data = p->data;
  int i;
  struct cee_json *error;

  for (i = 0; info[i].var_name; i++) {
    if (strcmp(key->_, info[i].col_name) == 0) {
      switch (info[i].type) {
        case CEE_SQLITE3_INT:
          if (cee_json_to_int(value, &data[i].i)) {
            cee_json_array_append_strf(p->used,"%s", info[i].col_name);
            data[i].has_value = 1;
          }
          else
            cee_json_array_append_strf(p->errors, "'%s' should be int",
                                       info[i].col_name);
          break;
        case CEE_SQLITE3_INT64:
          if (cee_json_to_i64(value, &data[i].i64)) {
            cee_json_array_append_strf(p->used,"%s", info[i].col_name);
            data[i].has_value = 1;
          }
          else
            cee_json_array_append_strf(p->errors, "'%s' should be int64",
                                       info[i].col_name);
          break;
        case CEE_SQLITE3_TEXT:
          if ((data[i].value = cee_json_to_str(value)->_)) {
            cee_json_array_append_strf(p->used,"%s", info[i].col_name);
            data[i].has_value = 1;
          }
          else
            cee_json_array_append_strf(p->errors, "'%s' should be text",
                                       info[i].col_name);
          break;
        case CEE_SQLITE3_BLOB: {
          // decode the string as base64 encoding
          cee_json_array_append_strf(p->errors, "'%s' should be blob and is not supported yet",
                                     info[i].col_name);
        }
          break;
      }
      if (p->update_set
          && !info[i].no_update
          && (data[i].has_value || info[i].data.has_value)) {
        if (strlen(p->update_set->_) == 0)
          p->update_set = cee_str_catf(p->update_set, "%s=%s", info[i].col_name, info[i].var_name);
        else
          p->update_set = cee_str_catf(p->update_set, ",%s=%s", info[i].col_name, info[i].var_name);
      }
      return;
    }
  }
  // report this value is ignored
  cee_json_array_append(p->unused, cee_json_str_mkf(p->state, "%s", key->_));
}

/*
 * @param json the input json
 * using the input json to populate data and run opcode
 */
int cee_sqlite3_generic_opcode(struct cee_state *st,
                   sqlite3 *db,
                   struct cee_json *json,
                   struct cee_sqlite3_bind_info *info,
                   struct cee_sqlite3_bind_data *data,
                   struct cee_sqlite3_stmt_strs *stmts,
                   struct cee_json **status,
                   int (*f)(struct cee_state *st,
                            sqlite3 *db,
                            struct cee_sqlite3_bind_info *info,
                            struct cee_sqlite3_bind_data *data,
                            struct cee_sqlite3_stmt_strs *stmts,
                            struct cee_json **status))
{
  struct _combo aaa = {
    .state = st,
    .json_in = json,
    .stmts = stmts,
    .data = data,
    .info = info,
    .update_set = NULL
  };

  struct cee_json *result = NULL;
  accept_result(st, status, &result);

  aaa.errors = cee_json_array_mk(st, 1);
  aaa.unused = cee_json_array_mk(st, 10);
  aaa.used = cee_json_array_mk(st, 10);
  if (stmts->update_template)
    aaa.update_set = cee_str_mk(st, "");
  
  /* populate aaa.data with the key/value pairs from json */
  cee_json_object_iterate(json, &aaa, populate_opcode);

  if (stmts->update_template)
    stmts->update_stmt_x = (char*)cee_str_mk(st, stmts->update_template, aaa.update_set);
  
  int rc = f (st, db, info, data, stmts, status);

  if (stmts->update_template) {
    cee_del(stmts->update_stmt_x);
    stmts->update_stmt_x = NULL;
  }

  if (result == NULL) return rc;
  
  struct cee_json *debug = cee_json_object_mk(st);
  cee_json_object_set(debug, "used_keys", aaa.used);

  if (cee_json_select(aaa.unused, "[0]"))
    cee_json_object_set(debug, "unused_keys", aaa.unused);
  else
    cee_del(aaa.unused);

  cee_json_object_set(result, "debug", debug);
  if (cee_json_select(aaa.errors, "[0]"))
    cee_json_object_set(result, "error", aaa.errors);
  else
    cee_del(aaa.errors);

  return rc;
}


struct cee_json* cee_sqlite3_get_selected(struct cee_json *status)
{
  return cee_json_select(status, ".sqlite3_selected");
}
