#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "cee.h"
#include "greatest.h"

TEST check_string_against_original(char *str)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_str *s = cee_str_mk(st, "%s", str);
  ASSERT_STR_EQ(str, (char *)s);
  cee_del(st);
  PASS();
}

TEST check_string_concatenation(void)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_str *s1, *s2, *s3;
  s1 = cee_str_mk(st, "%d", 10);
  ASSERT_STR_EQ("10", (char *)s1);
  s2 = cee_str_mk(st, "%.1f", 10.3);
  ASSERT_STR_EQ("10.3", (char *)s2);
  s3 = cee_str_mk(st, "%s %s", s1, s2);
  ASSERT_STR_EQ("10 10.3", (char *)s3);
  cee_del(st);
  PASS();
}

TEST check_list_append(void)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_list *list = cee_list_mk(st, 10);
  struct cee_str *s[] = { 
    cee_str_mk(st, "%s", "1"),
    cee_str_mk(st, "%s", "2"),
    cee_str_mk(st, "%s", "3")
  };
  const unsigned arr_len = sizeof(s) / sizeof(struct cee_str*);

  for (int i=0; i < arr_len; ++i) {
    cee_list_append(&list, s[i]);
  }
  ASSERT(cee_list_size(list) == arr_len);

  for (int i=0; i < cee_list_size(list); ++i) {
    char num[32];
    snprintf(num, sizeof(num), "%d", i+1);
    ASSERT_STR_EQ(num, (char *)list->_[i]);
  }
  cee_del(st);
  PASS();
}

TEST check_list_heterogenous(void)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_list *list = cee_list_mk(st, 10);

  /* heterogeneous list [ 10, 10.0, "10"] */
  enum T { I_T, F_T, S_T };
  cee_list_append(&list, cee_tagged_mk(st, I_T, cee_boxed_from_i32(st, 10)));
  cee_list_append(&list, cee_tagged_mk(st, F_T, cee_boxed_from_float(st, 10.1f)));
  cee_list_append(&list, cee_tagged_mk(st, S_T, cee_str_mk(st, "10")));
  ASSERT_EQ(10, cee_boxed_to_i32(list->_[0]));
  ASSERT_EQ(10.1f, cee_boxed_to_float(list->_[1]));
  ASSERT_STR_EQ("10", (char *)list->_[2]);
  cee_del(st);
  PASS();
}

void * baz (struct cee_state *st, struct cee_env * outer, size_t amt, va_list ap) {
  int u = va_arg(ap, int), v = va_arg(ap, int), w = va_arg(ap, int);

  int m = cee_boxed_to_i32(cee_env_find(outer, "m"));
  int n = cee_boxed_to_i32(cee_env_find(outer, "n"));

  int x = cee_boxed_to_i32(cee_env_find(outer, "x"));
  int y = cee_boxed_to_i32(cee_env_find(outer, "y"));

  return cee_boxed_from_i32(st, (u+m+x) + v*n*y + w);
}

void * bar (struct cee_state *st, struct cee_env * outer, size_t amt, va_list ap) {
  int i = va_arg(ap, int), j = va_arg(ap, int), k = va_arg(ap, int);

  struct cee_map * vars = cee_map_mk(st, (cee_cmp_fun)strcmp);
  cee_map_add(vars, cee_str_mk(st, "x"), cee_boxed_from_i32(st, 2));
  cee_map_add(vars, cee_str_mk(st, "y"), cee_boxed_from_i32(st, 5));
  struct cee_env * e = cee_env_mk(st, outer, vars);
  struct cee_closure * c = cee_closure_mk(st, e, &baz);

  return cee_boxed_from_i32(st, (i+j+k) * cee_boxed_to_i32(cee_closure_call(st, c, 3, 1, 2, 3)));
}


void f(void *ctx, void *key, void *value)
{
  struct cee_str *k = key;
  struct cee_boxed *v = value;
  printf ("key:%s->%d\n", k->_, cee_boxed_to_i32(v));
}

SUITE(cee_str)
{
  char *str_list[] = { "Hello World!", "Fish", "" , "\n\t" };
  for (int i=0; i < sizeof(str_list)/sizeof(char*); ++i) {
    RUN_TESTp(check_string_against_original, str_list[i]);
  }
  RUN_TEST(check_string_concatenation);
}

SUITE(cee_list)
{
  RUN_TEST(check_list_append);
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) 
{
  GREATEST_MAIN_BEGIN();

  RUN_SUITE(cee_str);
  RUN_SUITE(cee_list);

#if 0
  struct cee_state * st = cee_state_mk(10);
  
  /* test set */
  struct cee_set * set1 = NULL;
  set1 = cee_set_mk(st, (cee_cmp_fun)strcmp);
  
  printf ("st: %p\n", set1);
  cee_set_add(set1, cee_str_mk(st, "a"));
  cee_set_add(set1, cee_str_mk(st, "b"));
  cee_set_add(set1, cee_str_mk(st, "c"));  
  void * sx = cee_str_mk(st, "aabc");
  printf ("sx:%p\n", sx);
  cee_set_add(set1, sx);
  
  char * p = (char *)cee_set_find(set1, (char *)"aabc");
  printf ("%p %s\n", p, p);
  
  cee_set_remove(set1, cee_str_mk(st, "aabc"));
  p = (char *)cee_set_find(set1, (char *)"aabc");
  printf ("%p\n", p);
  
  struct cee_list * svals = NULL;
  svals = cee_set_values(set1);
  printf ("print out values of a set\n");
  for (i = 0; i < cee_list_size(svals); i++)
    printf ("%d %s\n", i, (char*)svals->_[i]);
  
  // optional
  // cee_del(set);
  // cee_del(svals);
  cee_state_add_gc_root(st, set1);
  
  /* test map */
  struct cee_map * mp = NULL;
  mp = cee_map_mk(st, (cee_cmp_fun)strcmp);
  
  cee_map_add(mp, cee_str_mk(st, "1"), cee_boxed_from_i32(st, 10));
  cee_map_add(mp, cee_str_mk(st, "2"), cee_boxed_from_i32(st, 20));
  cee_map_add(mp, cee_str_mk(st, "3"), cee_boxed_from_i32(st, 30));
  cee_map_add(mp, cee_str_mk(st, "1"), cee_boxed_from_i32(st, 100));
  
  struct cee_boxed * t = (struct cee_boxed *)cee_map_find(mp, "1");
  printf ("found value %d for key 1\n", cee_boxed_to_i32(t));
  if (cee_boxed_to_i32(t) != 100)  {
     printf("test failed\n");
     cee_segfault();
     return 1;
  }


  printf ("print out keys of a map\n");
  struct cee_list * keys = cee_map_keys(mp);
  for (i = 0; i < cee_list_size(keys); i++)
    printf ("[%d] key:%s\n", i, (char *)keys->_[i]);

  cee_map_iterate(mp, NULL, f);

  // optional
  //cee_del(keys);
  //cee_del(mp);
  cee_state_add_gc_root(st, mp);
  
  /* test stack */
  struct cee_stack * sp = cee_stack_mk(st, 100);
  cee_stack_push(sp, cee_str_mk(st, "1"));
  cee_stack_push(sp, cee_str_mk(st, "2"));
  cee_stack_push(sp, cee_str_mk(st, "3"));
  printf ("%s\n", (char*)cee_stack_top(sp, 0));
  
  // optional
  // cee_del(sp);
  cee_state_add_gc_root(st, sp);
  
  /* test diction */
  struct cee_dict * dict = cee_dict_mk(st, 100);
  
  for (i = 0; i < 1000; i++)
    cee_dict_add(dict, cee_str_mk(st, "%d", i)->_, cee_str_mk(st, "value %d", i));

  struct cee_str * key = cee_str_mk(st, "9");
  printf ("%s\n", (char*)cee_dict_find(dict, key->_));
  
  // optional
  // cee_del(key);
  // cee_del(dict);
  
  struct cee_n_tuple * t5 = 
    cee_n_tuple_mk(st, 5, cee_str_mk(st, "1"), cee_str_mk(st, "2"), cee_str_mk(st, "3"), 
                cee_str_mk(st, "4"), cee_str_mk(st, "5"));
  
  for (i = 0; i < 5; i++)
    printf("%d, %s\n", i, (char*)t5->_[i]);
  
  // cee_del(t5);
  printf("t5:%p\n", t5);
  cee_state_add_gc_root(st, t5);
  printf("%zu\n", cee_set_size(st->roots));
  
  struct cee_list * roots = cee_set_values(st->roots);
  for (i = 0; i < cee_list_size(roots); i++) {
    printf ("%d:%p\n", i, roots->_[i]);
  }
  
  struct cee_map * vars = cee_map_mk(st, (cee_cmp_fun)strcmp);
  cee_map_add(vars, cee_str_mk(st, "m"), cee_boxed_from_i32(st, 2));
  cee_map_add(vars, cee_str_mk(st, "n"), cee_boxed_from_i32(st, 1));
  struct cee_env *e = cee_env_mk(st, NULL, vars);
  struct cee_closure *c = cee_closure_mk(st, e, &bar);
  printf ("bar result:%d\n", cee_boxed_to_i32(cee_closure_call(st, c, 3, 1, 1, 1)));
  
  // optional
  cee_state_gc(st);
  
  cee_state_remove_gc_root(st, t5);
  printf("%zu\n", cee_set_size(st->roots));
  cee_state_gc(st);
  
  cee_state_remove_gc_root(st, mp);
  printf("%zu\n", cee_set_size(st->roots));
  cee_state_gc(st);
  
  cee_state_remove_gc_root(st, list);
  printf("%zu\n", cee_set_size(st->roots));
  cee_state_gc(st);
  
  cee_state_remove_gc_root(st, sp);
  printf("%zu\n", cee_set_size(st->roots));
  cee_state_gc(st);
  
  cee_state_remove_gc_root(st, set1);
  printf("%zu\n", cee_set_size(st->roots));
  cee_state_gc(st);
  
  cee_del(st);
  printf ("exit\n");
#endif

  GREATEST_MAIN_END();
}
