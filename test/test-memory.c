#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "release/cee.c"
#include "greatest.h"

#define N_APPENDS 1000

struct generic {
  enum { UNDEF=0, INT32, STRING, FLOAT } type;
  void *key;
  union {
    void *p;
    int   i;
    float f;
  };
};

typedef void (*add_n_check_fn)(struct cee_state *st, unsigned n_append, void **p_root);

void add_n_list(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_list *list = cee_list_mk(st, 10);
  for (unsigned i=0, j=0; i < n_append; ++i, ++j) {
    switch (j) {
    case 0: cee_list_append(list, cee_str_mk(st, "%u", i)); break;
    case 1: cee_list_append(list, cee_tagged_mk(st, FLOAT, cee_boxed_from_float(st, 1.0f * i))); break;
    case 2: cee_list_append(list, cee_tagged_mk(st, INT32, cee_boxed_from_i32(st, i))); break;
    case 3: cee_list_append(list, cee_tagged_mk(st, STRING, cee_str_mk(st, "%u", i))); j=0; break;
    }
  }
  cee_state_add_gc_root(st, list);
  *p_root = list;
}

void add_n_set(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_set *set = cee_set_mk(st, (cee_cmp_fun)&strcmp);
  for (unsigned i=0; i < n_append; ++i) {
    cee_set_add(set, cee_str_mk(st, "%u", i));
  }
  cee_state_add_gc_root(st, set);
  *p_root = set;
}

void add_n_map(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_map *map = cee_map_mk(st, (cee_cmp_fun)&strcmp);
  for (unsigned i=0, j=0; i < n_append; ++i, ++j) {
    struct cee_str *key = cee_str_mk(st, "%u", i);
    switch (j) {
    case 0: cee_map_add(map, key, cee_boxed_from_i32(st, i)); break;
    case 1: cee_map_add(map, key, cee_str_mk(st, "hello %u", i)); break;
    case 2: cee_map_add(map, key, cee_tagged_mk(st, FLOAT, cee_boxed_from_float(st, 1.0f * i))); break;
    case 3: cee_map_add(map, key, cee_tagged_mk(st, INT32, cee_boxed_from_i32(st, i))); break;
    case 4: cee_map_add(map, key, cee_tagged_mk(st, STRING, cee_str_mk(st, "%u", i))); j = 0; break;
    }
  }
  cee_state_add_gc_root(st, map);
  *p_root = map;
}

void add_n_stack(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_stack *s = cee_stack_mk(st, 100);
  for (unsigned i=0, j=0; i < n_append; ++i, ++j) {
    switch (j) {
    case 0: cee_stack_push(s, cee_boxed_from_i32(st, i)); break;
    case 1: cee_stack_push(s, cee_str_mk(st, "hello %u", i)); break;
    case 2: cee_stack_push(s, cee_tagged_mk(st, FLOAT, cee_boxed_from_float(st, 1.0f * i))); break;
    case 3: cee_stack_push(s, cee_tagged_mk(st, INT32, cee_boxed_from_i32(st, i))); break;
    case 4: cee_stack_push(s, cee_tagged_mk(st, STRING, cee_str_mk(st, "%u", i))); j = 0; break;
    }
  }
  cee_state_add_gc_root(st, s);
  *p_root = s;
}

void add_n_dict(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_dict *dict = cee_dict_mk(st, 100);
  for (unsigned i=0, j=0; i < n_append; ++i, ++j) {
    char *key = cee_str_mk(st, "%u", i)->_;
    switch (j) {
    case 0: cee_dict_add(dict, key, cee_boxed_from_i32(st, i)); break;
    case 1: cee_dict_add(dict, key, cee_str_mk(st, "hello %u", i)); break;
    case 2: cee_dict_add(dict, key, cee_tagged_mk(st, FLOAT, cee_boxed_from_float(st, 1.0f * i))); break;
    case 3: cee_dict_add(dict, key, cee_tagged_mk(st, INT32, cee_boxed_from_i32(st, i))); break;
    case 4: cee_dict_add(dict, key, cee_tagged_mk(st, STRING, cee_str_mk(st, "%u", i))); j = 0; break;
    }
  }
  cee_state_add_gc_root(st, dict);
  *p_root = dict;
}

TEST add_all__then_run_gc(const int n_appends)
{
  /* simply add new tests to the list */
  add_n_check_fn test_list[] = { add_n_list, add_n_set, add_n_map, add_n_stack, add_n_dict };
#define N_TESTS (sizeof(test_list) / sizeof(add_n_check_fn))

  struct cee_state *st = cee_state_mk(10);
  void *roots[N_TESTS] = {};
  for (unsigned i=0; i < N_TESTS; ++i) {
    test_list[i](st, n_appends, &roots[i]);
  } for (unsigned i=0; i < N_TESTS; ++i) {
    cee_state_gc(st);
    cee_state_remove_gc_root(st, roots[i]);
  }
  cee_del(st);
  PASS();
#undef N_TESTS
}

SUITE(garbage_collector)
{
  RUN_TESTp(add_all__then_run_gc, N_APPENDS);
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) 
{
  GREATEST_MAIN_BEGIN();

  RUN_SUITE(garbage_collector);

  GREATEST_MAIN_END();
}

