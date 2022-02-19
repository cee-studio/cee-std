#include "cee-sqlite3.h"
#include <stdlib.h>
struct join_ctx {
  struct cee_sqlite3 *cs;
  char *sql;
  struct cee_sqlite3_bind_info *pairs;
};

static int join_one (void *ctx, int idx, void *elem) {
  struct join_ctx *join_ctx = ctx;
  struct cee_sqlite3_bind_info *pairs = join_ctx->pairs;
  struct cee_json *one = elem;

  int i;
  for (i = 0; pairs[i].var_name; i++) {
    struct cee_json *value = cee_json_object_get(elem, pairs[i].col_name);
    if (NULL == value) {
      cee_json_object_set_strf(one, "error",
                               "cannot find json key %s", pairs[i].col_name);
      return 1; /* stop */
    }
    pairs[i].data.has_value = 1;
    switch(value->t) {
      case CEE_JSON_I64:
        if (!cee_json_to_int(value, &pairs[i].data.i)) {
          cee_json_object_set_strf(one, "error",
                                   "failed to convert json key %s's value to int", pairs[i].col_name);
          return 1; /* stop */
        }
        break; /* switch */
      case CEE_JSON_STRING:
        pairs[i].data.value = cee_json_to_str(value)->_;
        break; /* switch */
      default:
        /* change to unspported type errors */
        cee_json_object_set_strf(one, "error",
                                 "unsupported json key %s's value type %d",
                                 value->t);
        return 1; /* stop */
    }
  }
  cee_sqlite3_select1(join_ctx->cs, pairs, NULL, join_ctx->sql, &one);
  if (cee_json_select(one, ".error"))
    return 1;
  return 0;
}

void cee_sqlite3_json_array_join_table(struct cee_sqlite3 *cs,
                                       struct cee_json *json,
                                       struct cee_sqlite3_bind_info *pairs,
                                       char *sql) {
  struct join_ctx join_ctx = {0};
  join_ctx.cs = cs;
  join_ctx.sql = sql;
  join_ctx.pairs = pairs;
  int i;
  for (i = 0; pairs[i].var_name; i++)
    pairs[i].data.has_value = 0;
  struct cee_list *list = cee_json_to_array(json);
  cee_list_iterate(list, &join_ctx, join_one);
}


void cee_sqlite3_json_object_join_table(struct cee_sqlite3 *cs,
                                        struct cee_json *json,
                                        struct cee_sqlite3_bind_info *pairs,
                                        char *sql) {
  struct join_ctx join_ctx = {0};
  join_ctx.cs = cs;
  join_ctx.sql = sql;
  join_ctx.pairs = pairs;
  int i;
  for (i = 0; pairs[i].var_name; i++)
    pairs[i].data.has_value = 0;
  join_one(&join_ctx, 0, json);
}
