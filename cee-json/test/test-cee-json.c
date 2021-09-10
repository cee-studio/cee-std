#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cee-json.h"
#include "greatest.h"


static char** g_files;
static char** g_suffixes;
static int    g_n_files;
char          g_errbuf[1024];

char* load_whole_file(char *filename, size_t *filesize) 
{
  FILE *f = fopen(filename,"rb");
  assert(NULL != f && "Couldn't open file");

  fseek(f, 0, SEEK_END);
  *filesize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *str = malloc(*filesize);
  fread(str, 1, *filesize, f);
  fclose(f);

  return str;
}


GREATEST_MAIN_DEFS();

SUITE(cee_json);

TEST expect_decode(char str[], size_t len)
{
  struct cee_state * st = cee_state_mk(10);
  struct cee_json *json = NULL;

  int errline=-1;
  cee_json_parse(st, str, len, &json, false, &errline);
  if (errline != -1) {
    snprintf(g_errbuf, sizeof(g_errbuf), "Failed parsing at line: %d", errline);
    FAILm(g_errbuf);
  }
  PASS();
}

TEST expect_encode(void)
{
  SKIPm("TODO");
}

SUITE(json_decode)
{
  char*  jsonstr;
  size_t jsonlen;
  for (int i=0; i < g_n_files; ++i) {
    jsonstr = load_whole_file(g_files[i], &jsonlen);
    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TESTp(expect_decode, jsonstr, jsonlen);
    free(jsonstr);
  }
}

SUITE(json_encode)
{
  char*  jsonstr;
  size_t jsonlen;
  for (int i=0; i < g_n_files; ++i) {
    jsonstr = load_whole_file(g_files[i], &jsonlen);
    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TEST(expect_encode);
    free(jsonstr);
  }
}

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

    size_t size = end ? (end - start) : strlen(start) + 1;
    g_suffixes[i] = malloc(size);
    memcpy(g_suffixes[i], start, size);
  }

  RUN_SUITE(json_decode);
  RUN_SUITE(json_encode);

  GREATEST_MAIN_END();
}
