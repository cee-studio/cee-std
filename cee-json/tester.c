#include "json.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main () {
  struct cee_state * st = cee_state_mk(10);
  struct cee_json * js = cee_json_object_mk (st);
  
  cee_json_object_set_bool(st, js, "b", true);
  cee_json_object_set_bool(st, js, "b1", false);
  
  cee_json_object_set_string(st, js, "s1", "xxx\n");
  struct cee_json * js1 = cee_json_object_mk (st);
  cee_json_object_set_string(st, js1, "s2", "yyy");
  cee_json_object_set(st, js, "y1", js1);
  
  struct cee_json * js2 = cee_json_array_mk (st, 10);
  cee_json_array_append_string(st, js2, "false");
  cee_json_array_append_string(st, js2, "true");
  cee_json_object_set(st, js, "a1", js2);
  
  size_t jlen = cee_json_snprint (st, NULL, 0, js, 1);
  printf (" %zu\n", jlen);
  jlen = cee_json_snprint (st, NULL, 0, js, 0);
  printf (" %zu\n", jlen);
  
  char buf[1000];
  cee_json_snprint (st, buf, 109, js, 1);
  printf ("%s\n", buf);
  
  cee_json_snprint (st, buf, 109, js, 0);
  printf ("%s\n", buf);
  struct cee_json * result = NULL;
  int line;
  printf ("pasing\n");
  cee_json_parse(st, buf, jlen, &result, true, &line);
  printf ("end of parsing\n");
  
  cee_json_snprint (st, buf, 109, result, 0);
  printf ("parsed -> printed\n");
  printf ("%s\n", buf);

  cee_del(result);
  cee_del(js);
  cee_del(st);
  return 0;
}
