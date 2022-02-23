#include "cee-sqlite3.h"
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>

sqlite3* cee_sqlite3_init_db(char *dbname, char *sqlstmts)
{
  sqlite3 *db;

  int rc = sqlite3_open(dbname, &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "open %s: %s\n", dbname, sqlite3_errmsg(db));
    sqlite3_close(db);
    db = NULL;
    return NULL;
  }

  if (!sqlstmts)
    return db;

  char *err_msg=NULL;
  rc = sqlite3_exec(db, "begin transaction;", 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s:%s\n", dbname, err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return NULL;
  }

  rc = sqlite3_exec(db, sqlstmts, 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s:%s\n", dbname, err_msg);
    sqlite3_free(err_msg);
    sqlite3_close(db);
    return NULL;
  }

  rc = sqlite3_exec(db, "commit;", 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s:%s\n", dbname, err_msg);
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

void cee_sqlite3_init(struct cee_sqlite3 *x, char *db_name, struct cee_state *state, sqlite3 *db)
{
   x->db_name = db_name;
   x->state = state;
   x->db = db;
}

int cee_sqlite3_bind_run_sql(struct cee_sqlite3 *cs,
                             struct cee_sqlite3_bind_info *info,
                             struct cee_sqlite3_bind_data *data,
                             char *sql,
                             sqlite3_stmt **sql_stmt_pp,
                             struct cee_json **ret)
{
  struct cee_state *state = cs->state;
  sqlite3 *db = cs->db;
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
    cee_json_set_error(ret, "SQL:%s:[%d]'%s' -> %s", cs->db_name, rc, sql, sqlite3_errmsg(db));
  return rc;
}


static void accept_result(struct cee_state *state,
                          struct cee_json **status, struct cee_json **result_p) {
  if (status) {
    /* the caller wants to receive executions status and result */
    if (*status == NULL) 
      *status = cee_json_object_mk(state);

    *result_p = *status;
  }
  else
    *result_p = NULL;
}

int cee_sqlite3_update_or_insert(struct cee_sqlite3 *cs,
                                 struct cee_sqlite3_bind_info *info,
                                 struct cee_sqlite3_bind_data *data,
                                 struct cee_sqlite3_stmt_strs *stmts,
                                 struct cee_json **status)
{
  struct cee_state *state = cs->state;
  sqlite3 *db = cs->db;
  int rc;
  struct cee_json *result = NULL;
  accept_result(state, status, &result);
  
  rc = cee_sqlite3_bind_run_sql(cs, info, data, stmts->select_stmt, NULL, status);
  if (rc == SQLITE_ROW) {
    char *update = stmts->update_stmt ? stmts->update_stmt : stmts->update_stmt_x;
    if (!update) {
      if (status) {
        cee_json_object_set_strf(*status, "warning", "nothing to update %s", stmts->table_name);
        return SQLITE_DONE;
      }
      cee_segfault();
    }
    rc = cee_sqlite3_bind_run_sql(cs, info, data, update, NULL, status);
    if (rc != SQLITE_DONE)
      cee_json_set_error(status, "SQL:%s:[%d]'%s' -> %s", cs->db_name, rc, update, sqlite3_errmsg(db));
  }
  else {
    char *insert = stmts->insert_dynamic ? stmts->insert_stmt_x : stmts->insert_stmt;
    rc = cee_sqlite3_bind_run_sql(cs, info, data, insert, NULL, status);
    if (rc != SQLITE_DONE)
      cee_json_set_error(status, "SQL:%s:[%d]'%s' -> %s",
                         cs->db_name, rc, insert, sqlite3_errmsg(db));
    else if (result) {
      int row_id = sqlite3_last_insert_rowid(db);
      cee_json_object_set_i64(result, "last_insert_rowid", row_id);
    }
  }
  return rc;
}

int cee_sqlite3_insert(struct cee_sqlite3 *cs,
                       struct cee_sqlite3_bind_info *info,
                       struct cee_sqlite3_bind_data *data,
                       char *insert,
                       struct cee_json **status,
                       char *key)
{
  struct cee_state *state = cs->state;
  sqlite3 *db = cs->db;
  int rc;
  struct cee_json *result = NULL;

  accept_result(state, status, &result);
  rc = cee_sqlite3_bind_run_sql(cs, info, data, insert, NULL, status);
  if (rc != SQLITE_DONE)
    cee_json_set_error(status, "SQL:%s:[%d]'%s' -> %s", cs->db_name, rc, insert, sqlite3_errmsg(db));
  else if (result) {
    int row_id = sqlite3_last_insert_rowid(db);
    if (key)
      cee_json_object_set_i64(result, key, row_id);
    else
      cee_json_object_set_i64(result, "last_insert_rowid", row_id);
  }
  return rc;
}


int cee_sqlite3_delete(struct cee_sqlite3 *cs,
                       struct cee_sqlite3_bind_info *info,
                       struct cee_sqlite3_bind_data *data,
                       char *delete_sql,
                       struct cee_json **status,
		       char *key)
{
  struct cee_state *state = cs->state;
  sqlite3 *db = cs->db;
  int rc;
  struct cee_json *result = NULL;

  accept_result(state, status, &result);
  rc = cee_sqlite3_bind_run_sql(cs, info, data, delete_sql, NULL, status);
  if (rc != SQLITE_DONE)
    cee_json_set_error(status, "SQL:%s:[%d]'%s' -> %s",
		       cs->db_name, rc, delete_sql, sqlite3_errmsg(db));
  if (key)
    cee_json_object_set_i64(result, key, sqlite3_changes(cs->db));
  else
    cee_json_object_set_i64(result, "deleted", sqlite3_changes(cs->db));
  return rc;
}


int cee_sqlite3_update(struct cee_sqlite3 *cs,
                       struct cee_sqlite3_bind_info *info,
                       struct cee_sqlite3_bind_data *data,
                       struct cee_sqlite3_stmt_strs *stmts,
                       struct cee_json **status)
{
  int rc;
  rc = cee_sqlite3_update_if_exists(cs, info, data, stmts, status);
  if (rc == SQLITE_OK)
    cee_json_set_error(status, "SQL:%s:[%d]'%s' -> %s",
                       cs->db_name, rc, stmts->select_stmt,
                       sqlite3_errmsg(cs->db));
  return rc;
}

int cee_sqlite3_update_if_exists(struct cee_sqlite3 *cs,
                                struct cee_sqlite3_bind_info *info,
                                struct cee_sqlite3_bind_data *data,
                                struct cee_sqlite3_stmt_strs *stmts,
                                struct cee_json **status)
{
  int rc;
  struct cee_json *result = NULL;
  sqlite3 *db = cs->db;

  accept_result(cs->state, status, &result);

  rc = cee_sqlite3_bind_run_sql(cs, info, data, stmts->select_stmt, NULL, status);
  if (rc == SQLITE_ROW) {
    char *update = stmts->update_stmt ? stmts->update_stmt : stmts->update_stmt_x;
    if (!update) {
      if (status) {
        cee_json_object_set_strf(*status, "warning", "nothing to update %s", stmts->table_name);
        return SQLITE_DONE;
      }
      cee_segfault();
    }
    rc = cee_sqlite3_bind_run_sql(cs, info, data, update, NULL, status);
    if (rc != SQLITE_DONE)
      cee_json_set_error(status, "SQL:%s:[%d]'%s' -> %s", 
                         cs->db_name, rc, update, sqlite3_errmsg(db));
  }
  return rc;
}

static bool
dont_return_colum(struct cee_sqlite3_bind_info *info, char *colum_name)
{
  int i;
  if (!info) return false;
  for (i = 0; info[i].var_name || info[i].col_name; i++)
    if (info[i].col_name && strcmp(info[i].col_name, colum_name) == 0)
      return info[i].no_return;
  return false;
}

/*
 * returned value is a JSON array
 */
int cee_sqlite3_select(struct cee_sqlite3 *cs,
                       struct cee_sqlite3_bind_info *info,
                       struct cee_sqlite3_bind_data *data,
                       char *sql,
                       struct cee_json **status)
{
  if (status) {
    if (*status && (NULL == cee_json_to_array(*status)))
      /* we don't support *status is not an array */
      cee_segfault();
  }
  
  struct cee_state *state = cs->state;
  sqlite3 *db = cs->db;  
  sqlite3_stmt *stmt = NULL;
  struct cee_json *result = NULL, *my_status = NULL, *array = cee_json_array_mk(state, 1);
  
  int rc = cee_sqlite3_bind_run_sql(cs, info, data, sql, &stmt, &my_status);
  if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
    if (status) {
      accept_result(state, status, &result);
      cee_json_object_append(*status, "error", cee_json_select(my_status, ".error"));
      cee_json_set_error(status, "SQL:%s:[%d]'%s' %s", cs->db_name, rc, sql, sqlite3_errmsg(db));
    }
    return rc;
  }
  while (rc != SQLITE_DONE) {
    struct cee_json *obj = cee_json_object_mk(state);
    cee_json_array_append(array, obj);

    int i, num_cols = sqlite3_column_count(stmt);
    for (i = 0; i < num_cols; i++) {
      char *name = (char *)sqlite3_column_name(stmt, i);
      if (dont_return_colum(info, name)) continue;
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

  if (status) {
    if (*status)
      cee_json_merge(*status, array);
    else
      *status = array;
  }
  
  return rc;
}

/*
 * returned value is a JSON object
 */
int cee_sqlite3_select1(struct cee_sqlite3 *cs,
                        struct cee_sqlite3_bind_info *info,
                        struct cee_sqlite3_bind_data *data,
                        char *sql,
                        struct cee_json **status)
{
  struct cee_json *error, *result = NULL, *x = NULL;
  
  int rc = cee_sqlite3_select(cs, info, data, sql, &result);
  
  if ((error = cee_json_select(result, ".error"))) {
    if (status) {
      accept_result(cs->state, status, &x);
      cee_json_object_append(*status, "error", error);
    }
    return rc;
  }
  
  struct cee_json *one = cee_json_select(result, "[0]");
  if (one && status) {
    if (*status)
      cee_json_merge(*status, one);
    else
      *status = one;
  }
  return rc;
}

/*
 * returned value is a JSON array
 */
int cee_sqlite3_select_wrapper(struct cee_sqlite3 *cs,
                               struct cee_sqlite3_bind_info *info,
                               struct cee_sqlite3_bind_data *data,
                               struct cee_sqlite3_stmt_strs *stmts,
                               struct cee_json **status)
{
  return cee_sqlite3_select(cs, info, data, stmts->select_stmt, status);
}

int cee_sqlite3_insert_wrapper(struct cee_sqlite3 *cs,
                               struct cee_sqlite3_bind_info *info,
                               struct cee_sqlite3_bind_data *data,
                               struct cee_sqlite3_stmt_strs *stmts,
                               struct cee_json **status)
{
  char *insert = stmts->insert_dynamic ? stmts->insert_stmt_x : stmts->insert_stmt;
  return cee_sqlite3_insert(cs, info, data, insert, status, NULL);
}

int cee_sqlite3_select1_or_insert(struct cee_sqlite3 *cs,
                                  struct cee_sqlite3_bind_info *info,
                                  struct cee_sqlite3_bind_data *data,
                                  struct cee_sqlite3_stmt_strs *stmts,
                                  struct cee_json **status)
{
  struct cee_state *state = cs->state;
  sqlite3 *db = cs->db;  
  struct cee_json *result = NULL, *error;
  int rc = cee_sqlite3_select1(cs, info, data, stmts->select_stmt, &result);
  
  if (result) {
    if (status) {
      if ((error = cee_json_select(result, ".error"))) {
        if (*status)
          cee_json_object_append(*status, "error", error);
        else
          *status = result;
        return rc;
      }
      if (*status)
        cee_json_merge(*status, result);
      else
        *status = result;
    }
    return rc;
  }

  accept_result(state, status, &result);
  char *insert = stmts->insert_dynamic ? stmts->insert_stmt_x : stmts->insert_stmt;  
  rc = cee_sqlite3_bind_run_sql(cs, info, data, insert, NULL, status);

  if (rc != SQLITE_DONE)
    cee_json_set_error(status, "SQL:%s:[%d]'%s' -> %s",
                       cs->db_name, rc, insert, sqlite3_errmsg(db));
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
  struct cee_str *insert_colums;
  struct cee_str *insert_values;
};

static int
populate_usage(void *ctx, struct cee_str *key, struct cee_json *value) {
  struct _combo *p = ctx;
  struct cee_sqlite3_bind_info *info = p->info;
  struct cee_sqlite3_bind_data *data = p->data;
  int i;

  for (i = 0; info[i].var_name; i++) {
    if (strcmp(key->_, info[i].col_name) == 0) {
      switch (info[i].type) {
        case CEE_SQLITE3_INT:
          if (cee_json_to_int(value, &data[i].i)) {
            cee_json_array_append_strf(p->used,"%s", info[i].col_name);
            data[i].has_value = 1;
          }
          else {
            cee_json_array_append_strf(p->errors, "'%s' should be int",
                                       info[i].col_name);
            return 1;
          }
          break;
        case CEE_SQLITE3_INT64:
          if (cee_json_to_i64(value, &data[i].i64)) {
            cee_json_array_append_strf(p->used,"%s", info[i].col_name);
            data[i].has_value = 1;
          }
          else {
            cee_json_array_append_strf(p->errors, "'%s' should be int64",
                                       info[i].col_name);
            return 1;
          }
          break;
        case CEE_SQLITE3_TEXT:
          if ((data[i].value = cee_json_to_str(value)->_)) {
            cee_json_array_append_strf(p->used,"%s", info[i].col_name);
            data[i].has_value = 1;
          }
          else {
            cee_json_array_append_strf(p->errors, "'%s' should be text",
                                       info[i].col_name);
            return 1;
          }
          break;
        case CEE_SQLITE3_BLOB:
          // decode the string as base64 encoding
          cee_json_array_append_strf(p->errors, "'%s' should be blob and is not supported yet",
                                     info[i].col_name);
          return 1;
      }
      return 0;
    }
  }
  // report this value is ignored
  cee_json_array_append(p->unused, cee_json_str_mkf(p->state, "%s", key->_));
  return 0;
}


static void
compose_dyn_stmts(struct cee_json *input, struct _combo *p) {
  struct cee_sqlite3_bind_info *info = p->info;
  struct cee_sqlite3_bind_data *data = p->data;
  int i;

  for (i = 0; info[i].var_name; i++) {
    if (cee_json_object_get(input, info[i].col_name)) {
      if (p->update_set
          && !info[i].no_update
          && ((data && data[i].has_value) || info[i].data.has_value)) {

        if (strlen(p->update_set->_) == 0)
          p->update_set = cee_str_catf(p->update_set, "%s=%s", info[i].col_name, info[i].var_name);
        else
          p->update_set = cee_str_catf(p->update_set, ",%s=%s", info[i].col_name, info[i].var_name);
      }
    }

    if (p->insert_colums
        && ((data && data[i].has_value) || info[i].data.has_value || info[i].not_null)) {
      if (strlen(p->insert_colums->_) == 0) {
        p->insert_colums = cee_str_catf(p->insert_colums, "%s", info[i].col_name);
        p->insert_values = cee_str_catf(p->insert_values, "%s", info[i].var_name);
      }
      else {
        p->insert_colums = cee_str_catf(p->insert_colums, ",%s", info[i].col_name);
        p->insert_values = cee_str_catf(p->insert_values, ",%s", info[i].var_name);
      }
    }
  }
}

/*
 * @param json the input json
 * using the input json to populate data and run opcode
 */
int cee_sqlite3_generic_opcode(struct cee_sqlite3 *cs,
                               struct cee_json *json,
                               struct cee_sqlite3_bind_info *info,
                               struct cee_sqlite3_bind_data *data,
                               struct cee_sqlite3_stmt_strs *stmts,
                               struct cee_json **status,
                               int (*f)(struct cee_sqlite3 *cs,
                                        struct cee_sqlite3_bind_info *info,
                                        struct cee_sqlite3_bind_data *data,
                                        struct cee_sqlite3_stmt_strs *stmts,
                                        struct cee_json **status))
{
  struct cee_state *state = cs->state;
  struct _combo aaa = {
    .state = state,
    .json_in = json,
    .stmts = stmts,
    .data = data,
    .info = info,
    .update_set = NULL,
    .insert_colums = NULL,
    .insert_values = NULL
  };

  struct cee_json *result = NULL;
  accept_result(state, status, &result);

  aaa.errors = cee_json_array_mk(state, 1);
  aaa.unused = cee_json_array_mk(state, 10);
  aaa.used = cee_json_array_mk(state, 10);
  if (stmts->update_template)
    aaa.update_set = cee_str_mk(state, "");

  if (stmts->insert_dynamic) {
    aaa.insert_colums = cee_str_mk(state, "");
    aaa.insert_values = cee_str_mk(state, "");
  }

  /* populate aaa.data with the key/value pairs from json */
  if (aaa.info)
    cee_json_object_iterate(json, &aaa, populate_usage);
  else {
    if (NULL != aaa.data)
      cee_segfault();
    aaa.info = cee_json_to_bind_info(json);
  }
  compose_dyn_stmts(json, &aaa);

  if (stmts->update_template) {
    if (strlen(aaa.update_set->_))
      stmts->update_stmt_x = (char*)cee_str_mk(state, stmts->update_template, aaa.update_set);
    else
      stmts->update_stmt_x = NULL;
  }

  if (stmts->insert_dynamic)
    stmts->insert_stmt_x = (char*)cee_str_mk(state, "insert into %s (%s) values (%s)",
                                             stmts->table_name, aaa.insert_colums, aaa.insert_values);

  int rc = f (cs, aaa.info, data, stmts, status);

  struct cee_json *debug = cee_json_object_mk(state);
  if (stmts->update_template) {
    cee_json_object_set_strf(debug, "update", "%s", stmts->update_stmt_x);
    cee_del(stmts->update_stmt_x);
    stmts->update_stmt_x = NULL;
  }

  if (stmts->insert_dynamic) {
    cee_json_object_set_strf(debug, "insert", "%s", stmts->insert_stmt_x);
    cee_del(stmts->insert_stmt_x);
    stmts->insert_stmt_x = NULL;
  }

  if (result == NULL) return rc;
  
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


int cee_sqlite3_get_pragma_variable(sqlite3 *db, char *name) {
  sqlite3_stmt *sql_stmt = NULL;
  char buf[256];
  snprintf(buf, sizeof buf, "PRAGMA %s;", name);
  int rc = sqlite3_prepare_v2(db, buf, -1, &sql_stmt, 0);
  rc = sqlite3_step(sql_stmt);
  int value = sqlite3_column_int(sql_stmt, 0);
  sqlite3_finalize(sql_stmt);
  return value;
}



int cee_sqlite3_insert_op(struct cee_sqlite3 *cs,
                          struct cee_sqlite3_db_op *op,
                          struct cee_json *input,
                          struct cee_json **status) {
  int rc = cee_sqlite3_generic_opcode(cs, input,
                                      op->info,
                                      op->data,
                                      op->stmts,
                                      status,
                                      cee_sqlite3_insert_wrapper);
  cee_json_object_rename(*status, "last_insert_rowid", "id");
  return rc;
}

int cee_sqlite3_update_or_insert_op(struct cee_sqlite3 *cs,
                                    struct cee_sqlite3_db_op *op,
                                    struct cee_json *input,
                                    struct cee_json **status) {
  return cee_sqlite3_generic_opcode(cs, input,
                                    op->info,
                                    op->data,
                                    op->stmts,
                                    status,
                                    cee_sqlite3_update_or_insert);
}

int
cee_sqlite3_update_op(struct cee_sqlite3 *cs,
                      struct cee_sqlite3_db_op *op,
                      struct cee_json *input,
                      struct cee_json **status) {
  return cee_sqlite3_generic_opcode(cs, input,
                                    op->info,
                                    op->data,
                                    op->stmts,
                                    status,
                                    cee_sqlite3_update);
}

int
cee_sqlite3_update_if_exists_op(struct cee_sqlite3 *cs,
                                struct cee_sqlite3_db_op *op,
                                struct cee_json *input,
                                struct cee_json **status) {
  return cee_sqlite3_generic_opcode(cs, input,
                                    op->info,
                                    op->data,
                                    op->stmts,
                                    status,
                                    cee_sqlite3_update_if_exists);
}

int 
cee_sqlite3_select_op(struct cee_sqlite3 *cs,
                     struct cee_sqlite3_db_op *op,
                     struct cee_json *json,
                     struct cee_json **status) {
  return cee_sqlite3_generic_opcode(cs, json,
                                    op->info,
                                    op->data,
                                    op->stmts,
                                    status,
                                    cee_sqlite3_select_wrapper);
}


struct cee_sqlite3_db_op*
new_cee_sqlite3_db_op(struct cee_state *state,
                      struct cee_sqlite3_db_op *op,
                      size_t size_of_bind_infos)
{
  struct cee_sqlite3_db_op *new_op = cee_block_mk(state, sizeof (struct cee_sqlite3_db_op));
  memcpy(new_op, op, sizeof(*op));
  size_t s = sizeof(struct cee_sqlite3_bind_data) *
    size_of_bind_infos/(sizeof (struct cee_sqlite3_bind_info));
  new_op->data = cee_block_mk(state, s);
  memset(new_op->data, 0, s);
  return new_op;
}


int 
cee_sqlite3_select1_as(struct cee_sqlite3 *cs,
                       struct cee_sqlite3_bind_info *info,
                       struct cee_sqlite3_bind_data *data,
                       char *sql,
                       struct cee_json **status,
                       char *key) {
  
  struct cee_json *result = NULL, *one = NULL;
  int ret = cee_sqlite3_select1(cs, info, data, sql, &one);

  if (cee_json_select(one, ".error") && status) {
    *status = one;
    return ret;
  }

  accept_result(cs->state, status, &result);
  if (status) {
    struct cee_json *old = cee_json_object_get(*status, key);
    if (old)
      cee_json_merge(old, one);
    else
      old = one;
    cee_json_object_set(*status, key, old);
  }
  return ret;
}

int 
cee_sqlite3_select_as(struct cee_sqlite3 *cs,
                      struct cee_sqlite3_bind_info *info,
                      struct cee_sqlite3_bind_data *data,
                      char *sql,
                      struct cee_json **status,
                      char *key) {

  struct cee_json *result = NULL, *array = NULL;
  int ret = cee_sqlite3_select(cs, info, data, sql, &array);

  if (cee_json_select(array, ".error") && status) {
    if (*status)
      cee_json_object_append(*status, "error", cee_json_select(array, ".error"));
    else
      *status = array;
    return ret;
  }

  if (status) {
    if (*status)
      cee_json_object_append(*status, key, array);
    else
      *status = array;
  }
  return ret;
}


struct info_state {
  struct cee_state *state;
  struct cee_sqlite3_bind_info *info;
  int size;
};

static int f (void *cxt, struct cee_str *key, struct cee_json *val) {
  struct info_state *is = cxt;
  struct cee_sqlite3_bind_info *info = is->info;

  for (int i = 0; i < is->size; i++) {
    if (info[i].var_name) continue;
    switch (val->t) {
      case CEE_JSON_I64:
        if (!cee_json_to_int(val, &info[i].data.i))
          cee_segfault();
        info[i].type = CEE_SQLITE3_INT;
        break;
      case CEE_JSON_STRING:
        info[i].data.value = cee_json_to_str(val)->_;
        info[i].type = CEE_SQLITE3_TEXT;
        break;
      case CEE_JSON_NULL:
        return 0;
      default:
        fprintf(stderr, "%d", val->t);
        cee_segfault();
    }
    info[i].col_name = cee_str_mk(is->state,  "%s", key->_)->_;
    info[i].var_name = cee_str_mk(is->state, "@%s", key->_)->_;
    info[i].data.has_value = 1;
  }
  return 0;
}

struct cee_sqlite3_bind_info *
cee_json_to_bind_info(struct cee_json *input) {
  struct cee_state *state = cee_get_state(input->value.object);
  uintptr_t size = cee_map_size(cee_json_to_object(input));
  size_t m_size = (size + 1) * sizeof(struct cee_sqlite3_bind_info);
  struct cee_block *block = cee_block_mk(state, m_size);
  
  struct info_state is = { .state = state, .info = (void *)block, .size = size };
  memset(block->_, 0, m_size);
  cee_json_object_iterate(input, &is, f);
  return (void *)block;
}


int
cee_sqlite3_max_int(struct cee_sqlite3 *cs, char *int_column_name,
		    char *from_clause, struct cee_json **status) {
  char qx[128];
  snprintf(qx, sizeof qx, "select max(%s) as max_value %s;",
           int_column_name, from_clause);

  struct cee_json *input = NULL, *max_value = NULL;
  cee_sqlite3_select1(cs, NULL, NULL, qx, &input);
  if (cee_json_select(input, ".error")) {
    *status = input;
    return -1;
  }

  if ((max_value = cee_json_select(input, ".max_value:n"))) {
    int int_value = 0;
    if (cee_json_to_int(max_value, &int_value))
      return int_value;
  }
  return -1;
}
