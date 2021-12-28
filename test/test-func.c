#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>

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


TEST boxed__compare_values_by_type(void)
{
  struct cee_state *st = cee_state_mk(10);

  ASSERT_EQ(INT8_MAX, cee_boxed_to_i8(cee_boxed_from_i8(st, INT8_MAX)));
  ASSERT_EQ(INT8_MIN, cee_boxed_to_i8(cee_boxed_from_i8(st, INT8_MIN)));
  ASSERT_EQ(INT16_MAX, cee_boxed_to_i16(cee_boxed_from_i16(st, INT16_MAX)));
  ASSERT_EQ(INT16_MIN, cee_boxed_to_i16(cee_boxed_from_i16(st, INT16_MIN)));
  ASSERT_EQ(INT32_MAX, cee_boxed_to_i32(cee_boxed_from_i32(st, INT32_MAX)));
  ASSERT_EQ(INT32_MIN, cee_boxed_to_i32(cee_boxed_from_i32(st, INT32_MIN)));
  ASSERT_EQ(INT64_MAX, cee_boxed_to_i64(cee_boxed_from_i64(st, INT64_MAX)));
  ASSERT_EQ(INT64_MIN, cee_boxed_to_i64(cee_boxed_from_i64(st, INT64_MIN)));

  ASSERT_EQ(UINT8_MAX, cee_boxed_to_u8(cee_boxed_from_u8(st, UINT8_MAX)));
  ASSERT_EQ(UINT16_MAX, cee_boxed_to_u16(cee_boxed_from_u16(st, UINT16_MAX)));
  ASSERT_EQ(UINT32_MAX, cee_boxed_to_u32(cee_boxed_from_u32(st, UINT32_MAX)));
  ASSERT_EQ(UINT64_MAX, cee_boxed_to_u64(cee_boxed_from_u64(st, UINT64_MAX)));

  ASSERT_EQ(DBL_MAX, cee_boxed_to_double(cee_boxed_from_double(st, DBL_MAX)));
  ASSERT_EQ(DBL_MIN, cee_boxed_to_double(cee_boxed_from_double(st, DBL_MIN)));
  ASSERT_EQ(FLT_MAX, cee_boxed_to_float(cee_boxed_from_float(st, FLT_MAX)));
  ASSERT_EQ(FLT_MIN, cee_boxed_to_float(cee_boxed_from_float(st, FLT_MIN)));

  cee_del(st);
  PASS();
}

TEST boxed__print_values_by_type(void)
{
  struct cee_state *st = cee_state_mk(10);
  char buf[256], expect[256];

  snprintf(expect, sizeof(expect), "%"PRId8, INT8_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_i8(st, INT8_MAX));
  ASSERT_STR_EQ(expect, buf);
  snprintf(expect, sizeof(expect), "%"PRId16, INT16_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_i16(st, INT16_MAX));
  ASSERT_STR_EQ(expect, buf);
  snprintf(expect, sizeof(expect), "%"PRId32, INT32_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_i32(st, INT32_MAX));
  ASSERT_STR_EQ(expect, buf);
  snprintf(expect, sizeof(expect), "%"PRId64, INT64_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_i64(st, INT64_MAX));
  ASSERT_STR_EQ(expect, buf);

  snprintf(expect, sizeof(expect), "%"PRIu8, UINT8_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_u8(st, UINT8_MAX));
  ASSERT_STR_EQ(expect, buf);
  snprintf(expect, sizeof(expect), "%"PRIu16, UINT16_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_u16(st, UINT16_MAX));
  ASSERT_STR_EQ(expect, buf);
  snprintf(expect, sizeof(expect), "%"PRIu32, UINT32_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_u32(st, UINT32_MAX));
  ASSERT_STR_EQ(expect, buf);
  snprintf(expect, sizeof(expect), "%"PRIu64, UINT64_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_u64(st, UINT64_MAX));
  ASSERT_STR_EQ(expect, buf);

  snprintf(expect, sizeof(expect), "%lg", DBL_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_double(st, DBL_MAX));
  ASSERT_STR_EQ(expect, buf);
  snprintf(expect, sizeof(expect), "%g", FLT_MAX);
  cee_boxed_snprint(buf, sizeof(buf), cee_boxed_from_float(st, FLT_MAX));
  ASSERT_STR_EQ(expect, buf);
  cee_del(st);
  PASS();
}

TEST str__compare_initialized_against_original(char *str)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_str *s = cee_str_mk(st, "%s", str);
  ASSERT_STR_EQ(str, (char *)s);
  cee_del(st);
  PASS();
}

TEST str__concatenate_strings(void)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_str *s1, *s2, *s3;
  s1 = cee_str_mk(st, "%d", 10);
  ASSERT_STR_EQ("10", (char *)s1);
  s2 = cee_str_mk(st, "%.1f", 10.3);
  ASSERT_STR_EQ("10.3", (char *)s2);
  s3 = cee_str_mk(st, "%s %s", s1, s2);
  ASSERT_STR_EQ("10 10.3", (char *)s3);

  s1 = cee_str_mk(st, "");
  int i = 0;
  for (i = 0; i < 10; i++)
    s1 = cee_str_catf(s1, "%s = %s", "abc", "defg");

  cee_del(st);
  PASS();
}

TEST list__persistent_element_values_and_position(void)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_list *list = cee_list_mk(st, 10);
  struct cee_str *s[] = { 
    cee_str_mk(st, "%s", "1"),
    cee_str_mk(st, "%s", "2"),
    cee_str_mk(st, "%s", "3"),
    cee_str_mk(st, "%s", "4"),
    cee_str_mk(st, "%s", "5"),
    cee_str_mk(st, "%s", "6"),
    cee_str_mk(st, "%s", "7"),
    cee_str_mk(st, "%s", "8"),
    cee_str_mk(st, "%s", "9"),
    cee_str_mk(st, "%s", "10"),
    cee_str_mk(st, "%s", "11"),
    cee_str_mk(st, "%s", "12"),
    cee_str_mk(st, "%s", "13"),
  };
  const unsigned arr_len = sizeof(s) / sizeof(struct cee_str*);

  for (int i=0; i < arr_len; ++i) {
    cee_list_append(&list, s[i]);
  }
  ASSERT(cee_list_size(list) == arr_len);

  for (int i=0; i < cee_list_size(list); ++i) {
    char num[32];
    printf("%d\n", i);
    snprintf(num, sizeof(num), "%d", i+1);
    ASSERT_STR_EQ(num, (char *)list->_[i]);
  }
  cee_del(st);
  PASS();
}

TEST list__retrieve_heterogeneous_elements(void)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_list *list = cee_list_mk(st, 10);

  /* heterogeneous list [ 10, 10.1, "10"] */
  cee_list_append(&list, cee_tagged_mk(st, INT32, cee_boxed_from_i32(st, 10)));
  cee_list_append(&list, cee_tagged_mk(st, FLOAT, cee_boxed_from_float(st, 10.1f)));
  cee_list_append(&list, cee_tagged_mk(st, STRING, cee_str_mk(st, "10")));
  struct cee_tagged *tv = list->_[0];
  ASSERT_EQ(INT32, tv->tag);
  ASSERT_EQ(10, cee_boxed_to_i32(tv->ptr.boxed));
  tv = list->_[1];
  ASSERT_EQ(FLOAT, tv->tag);
  ASSERT_EQ(10.1f, cee_boxed_to_float(tv->ptr.boxed));
  tv = list->_[2];
  ASSERT_STR_EQ("10", (char*)tv->ptr.str);
  cee_del(st);
  PASS();
}

TEST set__find_element_by_value(char *str_list[], const unsigned n_str)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_set *set = cee_set_mk(st, (cee_cmp_fun)&strcmp);
  struct cee_list *set_values;
  char *p;

  for (unsigned i=0; i < n_str; ++i) {
    cee_set_add(set, cee_str_mk(st, str_list[i]));
  }

  set_values = cee_set_values(set);
  ASSERT(cee_set_size(set) == cee_list_size(set_values));

  for (unsigned i=0; i < cee_list_size(set_values); ++i) {
    p = cee_set_find(set, set_values->_[i]);
    ASSERT(p != NULL);
  }
  cee_del(st);
  PASS();
}


static void set_iter(void *cxt, void *v)
{
  struct cee_str *str = cxt;
  cee_str_catf(str, "%s", v);
}

TEST set__iterate(char *str_list[], const unsigned n_str)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_set *set = cee_set_mk(st, (cee_cmp_fun)&strcmp);
  struct cee_list *set_values;
  char *p;

  for (unsigned i=0; i < n_str; ++i)
    cee_set_add(set, cee_str_mk(st, str_list[i]));

  struct cee_str *str = cee_str_mk(st, "");

  cee_set_iterate(set, str, set_iter);
  ASSERT_STR_EQ(str->_, "Fish\n\tHello World!");
  cee_del(st);
  PASS();
}

TEST map__find_element_by_key(struct generic list[], const unsigned n)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_map *mp = cee_map_mk(st, (cee_cmp_fun)&strcmp);
  struct cee_boxed *t;

  for (unsigned i=0; i < n; ++i) {
    cee_map_add(mp, cee_str_mk(st, list[i].key), cee_boxed_from_i32(st, list[i].i));
  }
  for (unsigned i=0; i < n; ++i) {
    t = cee_map_find(mp, list[i].key);
    ASSERT(t != NULL);
  }
  cee_del(st);
  PASS();
}

TEST map__remove_element(struct generic list[], const unsigned n)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_map *mp = cee_map_mk(st, (cee_cmp_fun)&strcmp);
  struct cee_boxed *t;

  for (unsigned i=0; i < n; ++i) {
    cee_map_add(mp, cee_str_mk(st, list[i].key), cee_boxed_from_i32(st, list[i].i));
  }
  for (unsigned i=0; i < n; ++i) {
    t = cee_map_remove(mp, list[i].key);
    ASSERT(t != NULL);
  }
  cee_del(st);
  PASS();
}


static void map_iter_cb(void *ctx, void *p_key, void *p_value)
{
  struct cee_str *key = p_key;
  struct cee_boxed *value = p_value;
  //ASSERT(p_key != NULL);
  //ASSERT(p_value != NULL);
  //ASSERT(((void*)123) == ctx);
}

TEST map__iterate_by_keys(struct generic list[], const unsigned n)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_map *mp = cee_map_mk(st, (cee_cmp_fun)&strcmp);
  struct cee_list *keys;

  for (unsigned i=0; i < n; ++i) {
    cee_map_add(mp, cee_str_mk(st, list[i].key), cee_boxed_from_i32(st, list[i].i));
  }
  keys = cee_map_keys(mp);
  ASSERT(keys != NULL);
  ASSERT_EQ(n, cee_list_size(keys));
  /* @todo allow returning error value for cee_map_iterate() */
  cee_map_iterate(mp, (void*)123, &map_iter_cb);
  cee_del(st);
  PASS();
}

TEST map__overwrite_element_value(void)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_map *mp = cee_map_mk(st, (cee_cmp_fun)&strcmp);

  cee_map_add(mp, cee_str_mk(st, "1"), cee_boxed_from_i32(st, 10));
  cee_map_add(mp, cee_str_mk(st, "1"), cee_boxed_from_i32(st, 100));
  struct cee_boxed *t = cee_map_find(mp, "1");
  ASSERT_EQ(100, cee_boxed_to_i32(cee_map_find(mp, "1")));
  cee_del(st);
  PASS();
}

TEST stack__retrieve_pushed_elements(void)
{
  struct cee_state *st = cee_state_mk(10);
  struct cee_stack *sp = cee_stack_mk(st, 100);

  cee_stack_push(sp, cee_str_mk(st, "1"));
  cee_stack_push(sp, cee_str_mk(st, "2"));
  cee_stack_push(sp, cee_str_mk(st, "3"));
  ASSERT_STR_EQ("3", cee_stack_top(sp, 0));
  ASSERT_STR_EQ("2", cee_stack_top(sp, 1));
  ASSERT_STR_EQ("1", cee_stack_top(sp, 2));
  cee_del(st);
  PASS();
}

TEST dict__find_element_by_key(void)
{
  struct cee_state *st   = cee_state_mk(10);
  struct cee_dict  *dict = cee_dict_mk(st, 100);
  struct cee_str   *key;
  
  for (int i=0; i < 1000; ++i) {
    cee_dict_add(dict, cee_str_mk(st, "%d", i)->_, cee_str_mk(st, "value %d", i));
  }

  key = cee_str_mk(st, "9");
  ASSERT_STR_EQ("value 9", cee_dict_find(dict, key->_));
  cee_del(st);
  PASS();
}

TEST n_tuple__check_initializer(void)
{
  struct cee_state *st   = cee_state_mk(10);
  struct cee_n_tuple *t = 
    cee_n_tuple_mk(st, 5, cee_str_mk(st, "1"), cee_str_mk(st, "2"), cee_str_mk(st, "3"), cee_str_mk(st, "4"), cee_str_mk(st, "5"));
  
  for (int i=0; i < 5; ++i) {
    ASSERT_EQ(i+1, strtol(t->_[i], NULL, 10));
  }
  cee_del(st);
  PASS();
}

void* baz(struct cee_state *st, struct cee_env *outer, size_t amt, va_list ap) 
{
  int u = va_arg(ap, int), v = va_arg(ap, int), w = va_arg(ap, int);

  int m = cee_boxed_to_i32(cee_env_find(outer, "m"));
  int n = cee_boxed_to_i32(cee_env_find(outer, "n"));

  int x = cee_boxed_to_i32(cee_env_find(outer, "x"));
  int y = cee_boxed_to_i32(cee_env_find(outer, "y"));

  return cee_boxed_from_i32(st, (u+m+x) + v*n*y + w); // E
}

void* bar(struct cee_state *st, struct cee_env *outer, size_t amt, va_list ap) 
{
  int i = va_arg(ap, int), j = va_arg(ap, int), k = va_arg(ap, int);

  struct cee_map * vars = cee_map_mk(st, (cee_cmp_fun)strcmp);
  cee_map_add(vars, cee_str_mk(st, "x"), cee_boxed_from_i32(st, 2)); // C
  cee_map_add(vars, cee_str_mk(st, "y"), cee_boxed_from_i32(st, 5)); // D
  struct cee_env * e = cee_env_mk(st, outer, vars);
  struct cee_closure * c = cee_closure_mk(st, e, &baz);

  return cee_boxed_from_i32(st, (i+j+k) * cee_boxed_to_i32(cee_closure_call(st, c, 3, 1,2,3))); // F
}

TEST closure__check_persistent_state(void)
{
  /* analogous to the following JS script:
   * var m = 2;                              // A
   * var n = 1;                              // B
   * function bar(i, j, k) {
   *   var x = 2;                            // C
   *   var y = 5;                            // D
   *   function baz(u, v, w) {
   *     return (u+m+x) + v*n*y + w;         // E
   *   }
   *   return (i+j+k) * baz(1, 2, 3);        // F
   * }
   * // should return 54
   * bar(1, 1, 1);                           // G
   */
  struct cee_state *st   = cee_state_mk(10);
  struct cee_map   *vars = cee_map_mk(st, (cee_cmp_fun)&strcmp);
  cee_map_add(vars, cee_str_mk(st, "m"), cee_boxed_from_i32(st, 2)); // A
  cee_map_add(vars, cee_str_mk(st, "n"), cee_boxed_from_i32(st, 1)); // B
  struct cee_env     *e   = cee_env_mk(st, NULL, vars);
  struct cee_closure *c   = cee_closure_mk(st, e, &bar);
  struct cee_boxed   *ret = cee_closure_call(st, c, 3, 1,1,1);       // G
  ASSERT_EQ(54, cee_boxed_to_i32(ret));
  cee_del(st);
  PASS();
}

SUITE(cee_boxed)
{
  RUN_TEST(boxed__compare_values_by_type);
  RUN_TEST(boxed__print_values_by_type);
}

SUITE(cee_str)
{
  char *str_list[] = { "Hello World!", "Fish", "" , "\n\t" };

  for (unsigned i=0; i < sizeof(str_list)/sizeof(char*); ++i) {
    RUN_TESTp(str__compare_initialized_against_original, str_list[i]);
  }
  RUN_TEST(str__concatenate_strings);
}

SUITE(cee_list)
{
  RUN_TEST(list__persistent_element_values_and_position);
  RUN_TEST(list__retrieve_heterogeneous_elements);
}

SUITE(cee_set)
{
  char *str_list[] = { "Hello World!", "Fish", "" , "\n\t" };
  const unsigned n_str = sizeof(str_list)/sizeof(char*);

  RUN_TESTp(set__find_element_by_value, str_list, n_str);
  RUN_TESTp(set__iterate, str_list, n_str);
}

SUITE(cee_map)
{
  // type, key, value
  struct generic list[] = {
    { INT32, "1", (void*)10 },
    { INT32, "2", (void*)20 },
    { INT32, "3", (void*)30 }
  };
  const unsigned n_pairs = sizeof(list)/sizeof(struct generic);

  RUN_TESTp(map__find_element_by_key, list, n_pairs);
  RUN_TESTp(map__remove_element, list, n_pairs);
  RUN_TESTp(map__iterate_by_keys, list, n_pairs);
  RUN_TEST(map__overwrite_element_value);
}

SUITE(cee_stack)
{
  RUN_TEST(stack__retrieve_pushed_elements);
}

SUITE(cee_dict)
{
  RUN_TEST(dict__find_element_by_key);
}

SUITE(cee_n_tuple)
{
  RUN_TEST(n_tuple__check_initializer);
}

SUITE(cee_closure)
{
  RUN_TEST(closure__check_persistent_state);
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) 
{
  GREATEST_MAIN_BEGIN();

  RUN_SUITE(cee_boxed);
  RUN_SUITE(cee_str);
  RUN_SUITE(cee_list);
  RUN_SUITE(cee_set);
  RUN_SUITE(cee_map);
  RUN_SUITE(cee_stack);
  RUN_SUITE(cee_dict);
  RUN_SUITE(cee_n_tuple);
  RUN_SUITE(cee_closure);

  GREATEST_MAIN_END();
}
