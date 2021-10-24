#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "release/cee.c"
#include "greatest.h"

struct generic {
  enum { UNDEF=0, INT32, STRING, FLOAT } type;
  void *key;
  union {
    void *p;
    int   i;
    float f;
  };
};

float f_rand(float max_val) { return ((float)rand() / (float)RAND_MAX) * max_val; }

TEST add_list(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_list *list = cee_list_mk(st, 10);
  for (unsigned i=0; i < n_append; ++i) {
    switch (rand() % 4) {
    case 0: cee_list_append(&list, cee_str_mk(st, "%u", i)); break;
    case 1: cee_list_append(&list, cee_tagged_mk(st, FLOAT, cee_boxed_from_float(st, f_rand(i+1)))); break;
    case 2: cee_list_append(&list, cee_tagged_mk(st, INT32, cee_boxed_from_i32(st, rand() % (i+1)))); break;
    case 3: cee_list_append(&list, cee_tagged_mk(st, STRING, cee_str_mk(st, "%u", i))); break;
    }
  }
  cee_state_add_gc_root(st, list);
  *p_root = list;
  PASS();
}

TEST add_set(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_set *set = cee_set_mk(st, (cee_cmp_fun)&strcmp);
  for (unsigned i=0; i < n_append; ++i) {
    cee_set_add(set, cee_str_mk(st, "%u", i));
  }
  cee_state_add_gc_root(st, set);
  *p_root = set;
  PASS();
}

TEST add_map(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_map *map = cee_map_mk(st, (cee_cmp_fun)&strcmp);
  for (unsigned i=0; i < n_append; ++i) {
    struct cee_str *key = cee_str_mk(st, "%u", i);
    switch (rand() % 5) {
    case 0: cee_map_add(map, key, cee_boxed_from_i32(st, rand() % (i+1))); break;
    case 1: cee_map_add(map, key, cee_str_mk(st, "hello %u", i)); break;
    case 2: cee_map_add(map, key, cee_tagged_mk(st, FLOAT, cee_boxed_from_float(st, f_rand(i+1)))); break;
    case 3: cee_map_add(map, key, cee_tagged_mk(st, INT32, cee_boxed_from_i32(st, rand() % (i+1)))); break;
    case 4: cee_map_add(map, key, cee_tagged_mk(st, STRING, cee_str_mk(st, "%u", i))); break;
    }
  }
  cee_state_add_gc_root(st, map);
  *p_root = map;
  PASS();
}

TEST add_stack(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_stack *s = cee_stack_mk(st, 100);
  for (unsigned i=0; i < n_append; ++i) {
    switch (rand() % 5) {
    case 0: cee_stack_push(s, cee_boxed_from_i32(st, rand() % (i+1))); break;
    case 1: cee_stack_push(s, cee_str_mk(st, "hello %u", i)); break;
    case 2: cee_stack_push(s, cee_tagged_mk(st, FLOAT, cee_boxed_from_float(st, f_rand(i+1)))); break;
    case 3: cee_stack_push(s, cee_tagged_mk(st, INT32, cee_boxed_from_i32(st, rand() % (i+1)))); break;
    case 4: cee_stack_push(s, cee_tagged_mk(st, STRING, cee_str_mk(st, "%u", i))); break;
    }
  }
  cee_state_add_gc_root(st, s);
  *p_root = s;
  PASS();
}

TEST add_dict(struct cee_state *st, unsigned n_append, void **p_root)
{
  struct cee_dict *dict = cee_dict_mk(st, 100);
  for (unsigned i=0; i < n_append; ++i) {
    char *key = cee_str_mk(st, "%u", i)->_;
    switch (rand() % 5) {
    case 0: cee_dict_add(dict, key, cee_boxed_from_i32(st, rand() % (i+1))); break;
    case 1: cee_dict_add(dict, key, cee_str_mk(st, "hello %u", i)); break;
    case 2: cee_dict_add(dict, key, cee_tagged_mk(st, FLOAT, cee_boxed_from_float(st, f_rand(i+1)))); break;
    case 3: cee_dict_add(dict, key, cee_tagged_mk(st, INT32, cee_boxed_from_i32(st, rand() % (i+1)))); break;
    case 4: cee_dict_add(dict, key, cee_tagged_mk(st, STRING, cee_str_mk(st, "%u", i))); break;
    }
  }
  cee_state_add_gc_root(st, dict);
  *p_root = dict;
  PASS();
}

TEST state_cleanup(struct cee_state *st, void *roots[], unsigned n_roots)
{
  for (unsigned i=0; i < n_roots; ++i) {
    cee_state_gc(st);
    cee_state_remove_gc_root(st, roots[i]);
  }
  cee_del(st);
  PASS();
}

SUITE(cee_state)
{
  struct cee_state *st = cee_state_mk(10);
  void *roots[5]={};
  unsigned n_roots = sizeof(roots) / sizeof(void*);

  RUN_TESTp(add_list, st, 100, &roots[0]);
  RUN_TESTp(add_set, st, 100, &roots[1]);
  RUN_TESTp(add_map, st, 100, &roots[2]);
  RUN_TESTp(add_stack, st, 100, &roots[3]);
  RUN_TESTp(add_dict, st, 100, &roots[4]);

  RUN_TESTp(state_cleanup, st, roots, n_roots);
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) 
{
  GREATEST_MAIN_BEGIN();

  RUN_SUITE(cee_state);

  GREATEST_MAIN_END();
}

