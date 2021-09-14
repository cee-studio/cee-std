#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#include "cee-json.h"
#include "greatest.h"


static char** g_files;
static char** g_suffixes;
static int    g_n_files;
char          g_errbuf[2048];

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

TEST expect_decode(char str[], long len)
{
  struct cee_state * st = cee_state_mk(10);
  struct cee_json *json = NULL;

  pid_t pid = fork();
  if (pid < 0) goto _skip;

  if (pid == 0) { // child process
    int errline=-1;
    cee_json_parse(st, str, len, &json, false, &errline);
    _exit( (errline != -1) ? EXIT_FAILURE : EXIT_SUCCESS );
  }

  int status;
  wait(&status);
  if (!WIFEXITED(status) || WEXITSTATUS(status) == EXIT_FAILURE) {
    goto _fail;
  }

  PASS();

_skip:
  snprintf(g_errbuf, sizeof(g_errbuf), "%s", strerror(errno));
  SKIPm(g_errbuf);
_fail:
  snprintf(g_errbuf, sizeof(g_errbuf), "JSON: %.*s", (int)len, str);
  FAILm(g_errbuf);
}

TEST expect_encode(void)
{
  SKIPm("TODO");
}

SUITE(json_decode)
{
  char*  jsonstr;
  long fsize;
  for (int i=0; i < g_n_files; ++i) {
    jsonstr = load_whole_file(g_files[i], &fsize);
    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TESTp(expect_decode, jsonstr, fsize);
    free(jsonstr);
  }
}

SUITE(json_encode)
{
  char*  jsonstr;
  for (int i=0; i < g_n_files; ++i) {
    jsonstr = load_whole_file(g_files[i], NULL);
    greatest_set_test_suffix(g_suffixes[i]);
    RUN_TEST(expect_encode);
    free(jsonstr);
  }
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) {
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

  RUN_SUITE(json_decode);
  RUN_SUITE(json_encode);

  GREATEST_MAIN_END();
}
