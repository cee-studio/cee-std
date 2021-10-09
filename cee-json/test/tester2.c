#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cee-json.h"

int main () {
  struct cee_state *state = cee_state_mk(100);

#if 0
  char *buf = "{ \"a\":[[]], \"b\":5e10, \"c\":5e-10, \"d\":1.337, \"e\":-1 }";
#else
  char *buf = "{ \"f\":1, \"a\":[[]], \"b\":5e10, \"c\":5e-10, \"d\":1.337, \"e\":-1 }";
#endif

  struct cee_json *result = NULL;
  int line = 0;
  if (!cee_json_parse(state, buf, strlen(buf),  &result, true, &line)) {
    fprintf(stderr, "parsing error at line %d\n", line);
    return 0;
  }

  cee_json_asprint(state, &buf, result, 0);
  fprintf(stdout, "%s\n", buf);
}
