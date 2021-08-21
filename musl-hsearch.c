#ifdef CEE_AMALGAMATION
#undef   S
#define  S(f)  _cee_htable_##f
#else
#define  S(f)  _##f
#include <stdlib.h>
#include <string.h>
#include "musl-search.h"
#endif


/*
open addressing hash table with 2^n table size
quadratic probing is used in case of hash collision
tab indices and hash are size_t
after resize fails with ENOMEM the state of tab is still usable

with the posix api items cannot be iterated and length cannot be queried
*/

#define MINSIZE 8
#define MAXSIZE ((size_t)-1/2 + 1)

struct __tab {
  MUSL_ENTRY *entries;
  size_t mask;
  size_t used;
};

static struct musl_hsearch_data htab;

/*
static int musl_hcreate_r(size_t, struct musl_hsearch_data *);
static void musl_hdestroy_r(struct musl_hsearch_data *);
static int mul_hsearch_r(MUSL_ENTRY, ACTION, MUSL_ENTRY **, struct musl_hsearch_data *);
*/

static size_t keyhash(char *k)
{
  unsigned char *p = (unsigned char *)k;
  size_t h = 0;

  while (*p)
    h = 31*h + *p++;
  return h;
}

static int resize(size_t nel, struct musl_hsearch_data *htab)
{
  size_t newsize;
  size_t i, j;
  MUSL_ENTRY *e, *newe;
  MUSL_ENTRY *oldtab = htab->__tab->entries;
  MUSL_ENTRY *oldend = htab->__tab->entries + htab->__tab->mask + 1;

  if (nel > MAXSIZE)
    nel = MAXSIZE;
  for (newsize = MINSIZE; newsize < nel; newsize *= 2);
  htab->__tab->entries = (MUSL_ENTRY *)calloc(newsize, sizeof *htab->__tab->entries);
  if (!htab->__tab->entries) {
    htab->__tab->entries = oldtab;
    return 0;
  }
  htab->__tab->mask = newsize - 1;
  if (!oldtab)
    return 1;
  for (e = oldtab; e < oldend; e++)
    if (e->key) {
      for (i=keyhash(e->key),j=1; ; i+=j++) {
        newe = htab->__tab->entries + (i & htab->__tab->mask);
        if (!newe->key)
          break;
      }
      *newe = *e;
    }
  free(oldtab);
  return 1;
}

int musl_hcreate(size_t nel)
{
  return musl_hcreate_r(nel, &htab);
}

void musl_hdestroy(void)
{
  musl_hdestroy_r(&htab);
}

static MUSL_ENTRY *lookup(char *key, size_t hash, struct musl_hsearch_data *htab)
{
  size_t i, j;
  MUSL_ENTRY *e;

  for (i=hash,j=1; ; i+=j++) {
    e = htab->__tab->entries + (i & htab->__tab->mask);
    if (!e->key || strcmp(e->key, key) == 0)
      break;
  }
  return e;
}

MUSL_ENTRY *musl_hsearch(MUSL_ENTRY item, ACTION action)
{
  MUSL_ENTRY *e;
  musl_hsearch_r(item, action, &e, &htab);
  return e;
}

int musl_hcreate_r(size_t nel, struct musl_hsearch_data *htab)
{
  int r;

  htab->__tab = (struct __tab *) calloc(1, sizeof *htab->__tab);
  if (!htab->__tab)
    return 0;
  r = resize(nel, htab);
  if (r == 0) {
    free(htab->__tab);
    htab->__tab = 0;
  }
  return r;
}

void musl_hdestroy_r(struct musl_hsearch_data *htab)
{
  if (htab->__tab) free(htab->__tab->entries);
  free(htab->__tab);
  htab->__tab = 0;
}

int musl_hsearch_r(MUSL_ENTRY item, ACTION action, MUSL_ENTRY **retval, 
                   struct musl_hsearch_data *htab)
{
  size_t hash = keyhash(item.key);
  MUSL_ENTRY *e = lookup(item.key, hash, htab);

  if (e->key) {
    *retval = e;
    return 1;
  }
  if (action == FIND) {
    *retval = 0;
    return 0;
  }
  *e = item;
  if (++htab->__tab->used > htab->__tab->mask - htab->__tab->mask/4) {
    if (!resize(2*htab->__tab->used, htab)) {
      htab->__tab->used--;
      e->key = 0;
      *retval = 0;
      return 0;
    }
    e = lookup(item.key, hash, htab);
  }
  *retval = e;
  return 1;
}
