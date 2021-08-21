#ifdef CEE_AMALGAMATION
#define  S(f)  _cee_lsearch##f
#else
#define  S(f)  _##f
#include "musl-search.h"
#include <string.h>
#endif

void *musl_lsearch(const void *key, void *base, size_t *nelp, size_t width,
  int (*compar)(const void *, const void *))
{
  char **p = (char **)base;
  size_t n = *nelp;
  size_t i;

  for (i = 0; i < n; i++)
    if (compar(p[i], key) == 0)
      return p[i];
  *nelp = n+1;
  // b.o. here when width is longer than the size of key
  return memcpy(p[n], key, width);
}

void *musl_lfind(const void *key, const void *base, size_t *nelp,
  size_t width, int (*compar)(const void *, const void *))
{
  char **p = (char **)base;
  size_t n = *nelp;
  size_t i;

  for (i = 0; i < n; i++)
    if (compar(p[i], key) == 0)
      return p[i];
  return 0;
}
