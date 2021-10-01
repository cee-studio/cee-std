#ifndef MUSL_SEARCH_H
#define MUSL_SEARCH_H
#ifdef CEE_AMALGAMATION
#else
#include <stddef.h>
#endif

typedef enum { FIND, ENTER } ACTION;
typedef enum { preorder, postorder, endorder, leaf } VISIT;

typedef struct musl_entry {
	char *key;
	void *data;
} MUSL_ENTRY;

int musl_hcreate(size_t);
void musl_hdestroy(void);
MUSL_ENTRY *musl_hsearch(MUSL_ENTRY, ACTION);


struct musl_hsearch_data {
	struct __tab *__tab;
	unsigned int __unused1;
	unsigned int __unused2;
};

int musl_hcreate_r(size_t, struct musl_hsearch_data *);
void musl_hdestroy_r(struct musl_hsearch_data *);
int musl_hsearch_r(MUSL_ENTRY, ACTION, MUSL_ENTRY **, struct musl_hsearch_data *);

void musl_insque(void *, void *);
void musl_remque(void *);

void *musl_lsearch(const void *, void *, size_t *, size_t,
	int (*)(const void *, const void *));
void *musl_lfind(const void *, const void *, size_t *, size_t,
	int (*)(const void *, const void *));

void *musl_tdelete(void * cxt, const void *__restrict, void **__restrict, int(*)(void *, const void *, const void *));
void *musl_tfind(void * cxt, const void *, void *const *, int(*)(void *, const void *, const void *));
void *musl_tsearch(void * cxt, const void *, void **, int (*)(void *, const void *, const void *));
void musl_twalk(void * cxt, const void *, void (*)(void *, const void *, VISIT, int));


struct musl_qelem {
	struct qelem *q_forw, *q_back;
	char q_data[1];
};

void musl_tdestroy(void * cxt, void *, void (*)(void * cxt, void *));

#endif /* MUSL_SEARCH */

