#ifndef CEE_JSON_AMALGAMATION
#include <stdio.h>
#include "cee.h"
#include "cee-json.h"
#endif

int cee_json_merge_all_fileno(char **json_files, int json_file_count, int combined_fd){
  static char perror_buf[128] = {0};

  FILE *f0 = fopen(json_files[0], "r"), *fn, *f_combined;
  if( !f0 ){
    snprintf(perror_buf, sizeof perror_buf, "fopen(%s)", json_files[0]);
    perror(perror_buf);
    return 1;
  }
  struct cee_state *state = cee_state_mk(100);
  int line = -1;
  struct cee_json *j0, *jn;
  j0 = cee_json_load_from_FILE(state, f0, true, &line);
  if( j0 == NULL ){
    fprintf(stderr, "error at %s:%d\n", json_files[0], line);
    return 1;
  }
  fclose(f0);

  for( int i = 1; i < json_file_count; i++ ){
    fn = fopen(json_files[i], "r");
    if( !fn ){
      snprintf(perror_buf, sizeof perror_buf, "fopen(%s)", json_files[i]);
      perror(perror_buf);
      return 1;
    }
    line = -1;
    jn = cee_json_load_from_FILE(state, fn, true, &line);
    if( jn == NULL ){
      fprintf(stderr, "error at %s:%d\n", json_files[i], line);
      return 1;
    }
    cee_json_merge(j0, jn);
    fclose(fn);
  }

  struct cee_json *inputs = cee_json_array_mk(state, json_file_count);
  cee_json_object_set(j0, "originals", inputs);
  for( int i = 0; i < json_file_count; i++ ){
    struct cee_json *a = cee_json_str_mkf(state, "%s", json_files[i]);
    cee_json_array_append(inputs, a);
  }

  cee_json_save_fileno(state, j0, combined_fd, 1);
  cee_del(state);
  return 0;
}

int cee_json_merge_all(char **json_files, int json_file_count, char *merged){
  FILE *f_combined = fopen(merged, "w");
  if( !f_combined ){
    perror(merged);
    return 1;
  }
  int ret = cee_json_merge_all_fileno(json_files, json_file_count, fileno(f_combined));
  fclose(f_combined);
  return ret;
}
