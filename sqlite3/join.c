#include "cee-sqlite3.h"
#include <stdlib.h>
struct join_ctx {
  struct cee_sqlite3 *cs;
  char *sql;
  struct cee_sqlite3_bind_info *info;
  struct cee_sqlite3_bind_data *data;
  char *key;
};

static int join_one (void *ctx, void *elem, int idx) {
  struct join_ctx *join_ctx = ctx;
  struct cee_sqlite3_bind_info *info = join_ctx->info;
  struct cee_sqlite3_bind_data *data = join_ctx->data, *item = NULL;
  struct cee_json *one = elem;

  int i;
  for (i = 0; info[i].var_name; i++) {
    item = NULL;
    if (data && data[i].has_value)
      item = data + i;
    else if (info[i].data.has_value)
      item = &info[i].data;

    if (item == NULL) continue;

    struct cee_json *value = cee_json_object_get(elem, info[i].col_name);
    if (value == NULL) {
      cee_json_object_set_strf(one, "error", "cannot find json key %s for %s",
                               info[i].col_name, info[i].var_name);
      return 1; /* stop */
    }
    switch (value->t) {
      case CEE_JSON_I64:
        if( cee_json_to_intx(value, &item->i) ){
          cee_json_object_set_strf(
            one, "error", "failed to convert json key %s's value to int",
            info[i].col_name);
          return 1; /* stop */
        }
        break; /* switch */
      case CEE_JSON_STRING:
        item->value = cee_json_to_str(value)->_;
        break; /* switch */
      case CEE_JSON_NULL:
        cee_json_object_set_strf(
          one, "error", "unsupported json key %s's value is null",
          info[i].col_name);
        return 1; /* stop */
      default:
        /* change to unspported type errors */
        cee_json_object_set_strf(
          one, "error", "unsupported json key %s's value type %d",
          info[i].col_name, value->t);
        return 1; /* stop */
    }
  }
  if( join_ctx->key == NULL ){
    cee_sqlite3_select1(join_ctx->cs, info, data, join_ctx->sql, &one);
    if (cee_json_select(one, ".error"))
      return 1;
  }else{
    struct cee_json *rows = NULL, *error = NULL;
    cee_sqlite3_select(join_ctx->cs, info, data, join_ctx->sql, &rows);
    cee_json_object_set(one, join_ctx->key, rows);
    if( (error = cee_json_select(rows, ".error")) ){
      return 1;
    }
    
  }


  return 0;
}

void cee_sqlite3_json_array_join_table(struct cee_sqlite3 *cs,
                                       struct cee_json *json,
                                       struct cee_sqlite3_bind_info *info,
                                       struct cee_sqlite3_bind_data *data,
                                       char *sql, char *key) {
  struct join_ctx join_ctx = {0};
  join_ctx.key = key;
  join_ctx.cs = cs;
  join_ctx.sql = sql;
  join_ctx.info = info;
  join_ctx.data = data;
  struct cee_list *list = cee_json_to_array(json);
  cee_list_iterate(list, &join_ctx, join_one);
}

void cee_sqlite3_json_object_join_table(struct cee_sqlite3 *cs,
                                        struct cee_json *json,
                                        struct cee_sqlite3_bind_info *info,
                                        struct cee_sqlite3_bind_data *data,
                                        char *sql, char *key) {
  struct join_ctx join_ctx = {0};
  join_ctx.key = key;
  join_ctx.cs = cs;
  join_ctx.sql = sql;
  join_ctx.info = info;
  join_ctx.data = data;
  join_one(&join_ctx, json, 0);
}
