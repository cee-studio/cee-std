#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "release/cee.c"
#include "release/cee-json.c"

int main () {
  struct cee_state *state = cee_state_mk(100);

#if 0
  char *buf = "{ \"a\":[[]], \"b\":5e10, \"c\":5e-10, \"d\":1.337, \"e\":-1 }";
#else
  char *buf = "{ \"f\":{ \"f\":1, \"a\":true}, \"a\":[[]], \"b\":5e10, \"c\":5e-10, \"d\":1.337, \"e\":-1 }";
#endif

  struct cee_json *result = NULL;
  int line = 0;
  if (!cee_json_parse(state, buf, strlen(buf),  &result, true, &line)) {
    fprintf(stderr, "parsing error at line %d\n", line);
    return 0;
  }

  cee_json_asprint(state, &buf, NULL, result, 0);
  fprintf(stdout, "%s\n", buf);

  char *buf1 = "{ \"f\":{ \"f\":true} }";


  struct cee_json *result1 = NULL;
  if (!cee_json_parse(state, buf1, strlen(buf1), &result1, true, &line)) {
    fprintf(stderr, "parsing error at line %d\n", line);
    return 0;
  }
  cee_json_asprint(state, &buf1, NULL, result1, 0);
  fprintf(stdout, "%s\n", buf1);


  cee_json_merge(result, result1);
  cee_json_asprint(state, &buf, NULL, result, 0);
  fprintf(stdout, "%s\n", buf);
  cee_del(state);
}
