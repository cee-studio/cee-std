#ifdef CEE_AMALGAMATION
#undef S
#define S(x)  _musl_lsearch__##x
#else
#define  S(f)  _##f
#include "musl-search.h"
#endif

struct S(node) {
  struct S(node) *next;
  struct S(node) *prev;
};

void musl_insque(void *element, void *pred)
{
  struct S(node) *e = (struct S(node) *)element;
  struct S(node) *p = (struct S(node) *)pred;

  if (!p) {
    e->next = e->prev = 0;
    return;
  }
  e->next = p->next;
  e->prev = p;
  p->next = e;
  if (e->next)
    e->next->prev = e;
}

void musl_remque(void *element)
{
  struct S(node) *e = (struct S(node) *)element;

  if (e->next)
    e->next->prev = e->prev;
  if (e->prev)
    e->prev->next = e->next;
}
