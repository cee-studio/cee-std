#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "cJSON.h"
#include "cee-json.h"
#include "greatest.h"


static char **g_files;
static char **g_suffixes;
static int    g_n_files;

#define ERRBUF_SIZE 2048


char* load_whole_file(char *filename, long *p_fsize) 
{
  FILE *f = fopen(filename,"rb");
  assert(NULL != f && "Couldn't open file");

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *str = malloc(fsize + 1);

  str[fsize] = '\0';
  fread(str, 1, fsize, f);

  fclose(f);

  if (p_fsize) *p_fsize = fsize;

  return str;
}

size_t normalize_string(char **dest, char *src, size_t size)
{
  char *iter = *dest = malloc(size + 1);

  while (*src) {
    if (!isspace(*src))
      *iter++ = *src;
    ++src;
  }
  *iter = 0;
  return (size_t)(iter - *dest);
}

char* _check_cJSON(char str[], long len)
{
  char   *jsonstr = NULL;
  cJSON  *json    = NULL;

  json = cJSON_ParseWithLength(str, len);
  if (!json) return NULL;

  jsonstr = cJSON_PrintUnformatted(json);
  cJSON_Delete(json);
  return jsonstr;
}

char* _check_cee_json(char str[], long len)
{
  char   *jsonstr = NULL;
  ssize_t jsonlen = 0;
  struct cee_state *st      = cee_state_mk(10);
  struct cee_json  *json    = NULL;
  int               errline = -1;

  cee_json_parse(st, str, len, &json, true, &errline);
  if (errline != -1) return NULL;

  cee_json_asprint(st, &jsonstr, json, CEE_JSON_FMT_COMPACT);
  return jsonstr;
}

TEST check_cJSON(char str[], long len)
{
  static char errbuf[ERRBUF_SIZE];

  char  *normstr  = NULL;
  size_t normlen  = normalize_string(&normstr, str, len);
  if (!normstr) SKIPm("Couldn't normalize JSON");

  char *jsonstr = _check_cJSON(normstr, normlen);
  if (!jsonstr) SKIPm("Couldn't validate JSON");

  if (strcmp(normstr, jsonstr)) {
    snprintf(errbuf, sizeof(errbuf), "\nExpected: %.*s\nGot: %s", 
      (int)normlen, normstr, jsonstr);
    FAILm(errbuf);
  }
  PASS();
}

TEST check_cee_json(char str[], long len)
{
  static char errbuf[ERRBUF_SIZE];

  char  *normstr  = NULL;
  size_t normlen  = normalize_string(&normstr, str, len);
  if (!normstr) SKIPm("Couldn't normalize JSON");

  char *jsonstr = _check_cee_json(normstr, normlen);
  if (!jsonstr) SKIPm("Couldn't validate JSON");

  if (strcmp(normstr, jsonstr)) {
    snprintf(errbuf, sizeof(errbuf), "\nExpected: %.*s\nGot: %s", 
      (int)normlen, normstr, jsonstr);
    FAILm(errbuf);
  }
  PASS();
}

TEST cmp_cJSON_n_cee_json(char str[], long len)
{
  static char errbuf[ERRBUF_SIZE];

  char  *normstr  = NULL;
  size_t normlen  = normalize_string(&normstr, str, len);
  if (!normstr) SKIPm("Couldn't normalize JSON");

  char *jsonstr1 = _check_cJSON(normstr, normlen);
  if (!jsonstr1) SKIPm("Couldn't validate JSON");

  char *jsonstr2 = _check_cee_json(normstr, normlen);
  if (!jsonstr2) SKIPm("Couldn't validate JSON");

  if (strcmp(jsonstr1, jsonstr2)) {
    snprintf(errbuf, sizeof(errbuf), "\ncJSON: %s\ncee_json: %s", 
      jsonstr1, jsonstr2);
    FAILm(errbuf);
  }
  PASS();
}

SUITE(json_parsing)
{
  char* jsonstr;
  long  jsonlen;

  for (int i=0; i < g_n_files; ++i) {
    if (g_suffixes[i][0] != 'y')
      continue;

    jsonstr = load_whole_file(g_files[i], &jsonlen);

    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TESTp(check_cJSON, jsonstr, jsonlen);
    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TESTp(check_cee_json, jsonstr, jsonlen);
    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TESTp(cmp_cJSON_n_cee_json, jsonstr, jsonlen);

    free(jsonstr);
  }
}

SUITE(json_transform)
{
  char *jsonstr;
  long  jsonlen;

  for (int i=0; i < g_n_files; ++i) {
    jsonstr = load_whole_file(g_files[i], &jsonlen);
    
    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TESTp(check_cJSON, jsonstr, jsonlen);
    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TESTp(check_cee_json, jsonstr, jsonlen);
    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TESTp(cmp_cJSON_n_cee_json, jsonstr, jsonlen);

    free(jsonstr);
  }
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) 
{
  GREATEST_MAIN_BEGIN();

  for (int i=0; i < argc; ++i) {
    // we're assuming the files are after the "--" arg
    if (0 == strcmp("--", argv[i]) && (i+1 < argc)) {
      g_files   = argv + (i+1);
      g_n_files = argc - (i+1);
      break;
    }
  }
  assert(g_n_files != 0 && "Couldn't locate files");

  // create test suffixes for easy identification
  g_suffixes = malloc(g_n_files * sizeof(char *));
  char *start, *end;
  for (int i=0; i < g_n_files; ++i) {
    if ( (start = strchr(g_files[i], '/')) )
      ++start;
    else 
      start = g_files[i];
    end = strrchr(start, '.');

    size_t size = end ? (end - start) : strlen(start);
    g_suffixes[i] = malloc(size + 1);
    memcpy(g_suffixes[i], start, size);
    g_suffixes[i][size] = '\0';
  }

  RUN_SUITE(json_parsing);
  RUN_SUITE(json_transform);

  GREATEST_MAIN_END();
}
