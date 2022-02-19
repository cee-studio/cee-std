#include "cee-sqlite3.h"
#include <stdlib.h>
struct join_cxt {
  struct cee_sqlite3 *cs;
  char *sql;
  struct cee_sqlite3_bind_info *pairs;
};

static int join_one (void *cxt, int idx, void *elem) {
  struct join_cxt *join_cxt = cxt;
  struct cee_sqlite3_bind_info *pairs = join_cxt->pairs;
  struct cee_json *one = elem;

  int i;
  for (i = 0; pairs[i].var_name; i++) {
    struct cee_json *value = cee_json_object_get(elem, pairs[i].col_name);
    if (NULL == value)
      cee_segfault();
    switch(value->t) {
      case CEE_JSON_I64:
        if (!cee_json_to_int(value, &pairs[i].data.i))
          cee_segfault();
        break;
      case CEE_JSON_STRING:
        pairs[i].data.value = cee_json_to_str(value)->_;
        break;
      default:
        /* change to unspported type errors */
        cee_segfault();
    }
  }
  cee_sqlite3_select1(join_cxt->cs, pairs, NULL, join_cxt->sql, &one);
  if (cee_json_select(one, ".error"))
    return 1;
  return 0;
}

void cee_sqlite3_join_json_table(struct cee_sqlite3 *cs,
                                 struct cee_list *list,
                                 struct cee_sqlite3_bind_info *pairs,
                                 char *sql) {
  struct cee_sqlite3_bind_info *info = NULL;
  int i;

  struct join_cxt join_cxt = {0};
  join_cxt.cs = cs;
  join_cxt.sql = sql;
  join_cxt.pairs = info;
  cee_list_iterate(list, &join_cxt, join_one);
}