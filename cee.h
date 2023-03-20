#ifndef CEE_H
#define CEE_H

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

/*
 * for operations that might fail, this is aligned with
 * how shell treats the exit of a process.
 */
enum cee_status {
  cee_success = 0,
  cee_failure = 100
};

struct cee_state; /* forwarding */

typedef uintptr_t cee_tag_t;
typedef int (*cee_cmp_fun) (const void *, const void *);

enum cee_resize_method {
  CEE_RESIZE_WITH_IDENTITY = 0, /* resize with identity function */
  CEE_RESIZE_WITH_MALLOC = 1,   /* resize with malloc  (safe, but leak) */
  CEE_RESIZE_WITH_REALLOC = 2   /* resize with realloc (probably unsafe) */
};

enum cee_trace_action {
  CEE_TRACE_DEL_NO_FOLLOW = 0,
  CEE_TRACE_DEL_FOLLOW, /* trace points-to graph and delete each destination node */
  CEE_TRACE_MARK,       /* trace points-to graph and mark each node */
};




/*
 * A cotainer is an instance of struct cee_*
 * A cee element is an instance of struct cee_*
 * 
 * 
 * A container has one of the three delete policies, which dedicate
 * how the elements of the container will be handled once the container is 
 * deleted (freed).
 * 
 * CEE_DP_DEL_RC: if a container is freed, its cee element's in-degree will be 
 *         decreased by one. If any cee element's in-degree is zero, the element 
 *         will be freed. It's developer's responsibility to prevent cyclically
 *         pointed containers from having this policy.
 * 
 * CEE_DP_DEL: if a container is freed, all its cee elements will be freed 
 *         immediately. It's developer's responsiblity to prevent an element is 
 *         retained by multiple containers from having this policy.
 *
 * CEE_DP_NOOP: if a container is freed, nothing will happen to its elements.
 *         It's developer's responsiblity to prevent memory leaks.
 *
 * The default del_policy is CEE_DP_DEL_RC
 */
enum cee_del_policy {
  CEE_DP_DEL_RC = 0,
  CEE_DP_DEL = 1,
  CEE_DP_NOOP = 2
};

/*
 *
 * if an object is owned an del_immediate container, retained is 1, and 
 * in_degree is ignored.
 *
 * if an object is owned by multiple del_rc containers, in_degree is the 
 * number of containers.
 *
 */
struct cee_sect {
  uint8_t  cmp_stop_at_null:1;  /* 0: compare all bytes, otherwise stop at '\0' */
  uint8_t  resize_method:2;     /* three values: identity, malloc, realloc */
  uint8_t  retained:1;          /* if it is retained, in_degree is ignored */
  uint8_t  gc_mark:2;           /* used for mark & sweep gc */
  uint8_t  n_product;           /* n-ary (no more than 256) product type */
  uint16_t in_degree;           /* the number of cee objects points to this object */
  /* begin of gc fields */
  struct cee_state * state;     /* the gc state under which this block is allocated */
  struct cee_sect * trace_next; /* used for chaining struct cee_sect to be traced */
  struct cee_sect * trace_prev; /* used for chaining struct cee_sect to be traced */
  /* end of gc fields */
  uintptr_t mem_block_size;     /* the size of a memory block enclosing this struct */
  void *cmp;                    /* compare two memory blocks */
  
  /* the object specific generic scan function */
  /* it does memory deallocation, reference count decreasing, or liveness marking */
  void (*trace)(void *, enum cee_trace_action);
};

/*
 * A consecutive memory block of unknown length.
 * It can be safely casted to char *, but it may not 
 * be terminated by '\0'.
 */
struct cee_block {
  char _[1]; /* an array of chars */
};

/*
 * n: the number of bytes
 * the function performs one task
 * -- allocate a memory block to include at least n consecutive bytes
 * 
 * return: the address of the first byte in consecutive bytes, the address 
 *         can be freed by cee_del
 */
extern void * cee_block_mk (struct cee_state * s, size_t n);

/*
 * @param init_f: a function to initialize the allocated block
 */
void * cee_block_mk_e (struct cee_state *s, size_t n, void *cxt, void (*init_f)(void *cxt, void *block));


size_t cee_block_size (struct cee_block *b);

/*
 *
 * C string is an array of chars, it may or may not be terminated by '\0'.
 *
 * struct cee_str behavors extract like an array of chars, all standard 
 * string.h functions can be directly applied to it without any wrappers
 *
 * 
 * if it's not terminated by null strlen will read memory out of its bounds.
 *
 */
struct cee_str {
  char _[1];
};


/*
 * the function performs the following task
 * 1  allocate a memory block to include enough consecutive bytes
 * 2. initialize the consecutive bytes as a null terminated string
 *    with fmt and its arguments
 * 
 * return: the start address of the consecutive bytes that is 
 *         null termianted and strlen is 0.
 *         the address can be safely casted to struct cee_block *
 *
 * e.g.
 *
 *      allocate an empty string
 *      cee_str_mk (state, ""); 
 * 
 *      allocate a string for int 10
 *      cee_str_mk (state, "%d", 10);
 *
 */
extern struct cee_str * cee_str_mk (struct cee_state *s, const char *fmt, ...);
extern struct cee_str * cee_str_mkv (struct cee_state *s, const char *fmt, va_list ap);

/*
 * the function performs the following task
 * 1  allocate a memory block to include n consecutive bytes
 * 2. initialize the consecutive bytes with fmt and its arguments
 * 
 * return: the start address of the consecutive bytes that is
 *         null terminated.
 *         the address can be safely casted to struct cee_block *
 * e.g.
 *      allocate a string buffer of 100 bytes, and initialize it with 
 *      an empty string.
 *      cee_str_mk_e (state, 100, "");
 * 
 *      allocate a string buffer of 100 bytes and initialize it with
 *      an integer
 *      cee_str_mk_e (state, 100, "%d", 10);
 *
 */
extern struct cee_str  * cee_str_mk_e (struct cee_state * s, size_t n, const char * fmt, ...);

/*
 * return the pointer of the null terminator;
 * if the array is not null terminated, 
 * NULL is returned.
 */
extern char * cee_str_end (struct cee_str *);

/*
 * str: points to the begin of a struct cee_block
 *
 * the function performs one task
 * 1. add any char to the end of str
 *
 * return: the start address of a cee_block, a new cee_block will
 *         be allocated if the cee_block is too small.
 */
extern struct cee_str * cee_str_add (struct cee_str * str, char);

/*
 * just like the standard strcat, but this function composes the src
 * string through a fmt string and its varadic arguments.
 */
extern struct cee_str * cee_str_catf (struct cee_str *, const char * fmt, ...);
extern struct cee_str * cee_str_ncat (struct cee_str *, char * s, size_t);


/*
 * replace the existing string with a new string
 */
extern struct cee_str * cee_str_replace (struct cee_str *, const char *fmt, ...);


/*
 * trim(remove) all white spaces from the right side of a string, 
 * i.e. remove all trailing white spaces
 */
extern void cee_str_rtrim(struct cee_str *);

/*
 * trim(remove) all white spaces from the left side of a string, 
 * i.e. remove all leading white spaces
 */
extern void cee_str_ltrim(struct cee_str *);


extern struct cee_str* cee_str_replace_at_offset(struct cee_str *str, size_t offset, size_t len, char *s);

extern char*
str_replace_all(char *str, const char * old_substr, const char * new_substr);

extern char* str_replace_all_ext(char *str, int n_pairs, ...);


struct cee_strview {
  size_t count;
  const char *data;
};






/* an auto expandable list */
struct cee_list {
  void* *a;
};

/*
 * capacity: the initial capacity of the list
 * when the list is deleted, its elements will be handled by 
 * the default deletion policy
 */
extern struct cee_list * cee_list_mk (struct cee_state * s, size_t capacity);

/*
 *
 */
extern struct cee_list * cee_list_mk_e (struct cee_state * s, enum cee_del_policy o, size_t size);

/*
 * it may return a new list if the parameter list is too small
 */
extern void cee_list_append(struct cee_list *v, void * e);


/*
 * it inserts an element e at index and shift the rest elements 
 * to higher indices
 */
extern void cee_list_insert(struct cee_list *l, int index, void *e);

/*
 * it removes an element at index and shift the rest elements
 * to lower indices.
 * if the list is NULL, return false
 */
extern bool cee_list_remove(struct cee_list *v, int index);

/*
 * returns the number of elements in the list
 * if the list is null, return 0.
 */
extern size_t cee_list_size(struct cee_list * v);

/*
 * returns the capcity of the list
 * if the list is null, return 0.
 */
extern size_t cee_list_capacity (struct cee_list *);


/*
 * applies f to each element of the list with cxt
 * if the list is null, return immediately
 */
extern int cee_list_iterate (struct cee_list *, void *ctx, int (*f)(void *cxt, void *e, int idx));

extern void cee_list_merge (struct cee_list *dest, struct cee_list *src);
  
struct cee_tuple {
  void * _[2];
};


/*
 * construct a tuple from its parameters
 * v1: the first value of the tuple
 * v2: the second value of the tuple
 */
extern struct cee_tuple * cee_tuple_mk (struct cee_state * s, void * v1, void * v2);
extern struct cee_tuple * cee_tuple_mk_e (struct cee_state * s, 
                           enum cee_del_policy o[2], void * v1, void * v2);

/*
 * update the delete policy of each element of a tupple
 * index: 0 or 1
 * v:  delete policy
 * 
 */
extern void cee_tuple_update_del_policy(struct cee_tuple *t,
					int index,
					enum cee_del_policy v);

struct cee_triple {
  void * _[3];
};

/* 
 * construct a triple from its parameters
 * v1: the first value of the triple
 * v2: the second value of the triple
 * v3: the third value of the triple
 * when the triple is deleted, its elements will not be deleted
 */
extern struct cee_triple * cee_triple_mk(struct cee_state * s, void * v1, void * v2, void * v3);
extern struct cee_triple * cee_triple_mk_e(struct cee_state * s, 
                           enum cee_del_policy o[3], void * v1, void * v2, void * v3);

  
struct cee_quadruple {
  void * _[4];
};

/* 
 * construct a triple from its parameters
 * v1: the first value of the quaruple
 * v2: the second value of the quaruple
 * v3: the third value of the quadruple
 * v4: the fourth value of the quadruple
 * when the quadruple is deleted, its elements will not be deleted
 */
extern struct cee_quadruple * cee_quadruple_mk(struct cee_state * s, 
                            void * v1, void * v2, void * v3, void * v4);

extern struct cee_quadruple * cee_quadruple_mk_e(struct cee_state * s, 
                              enum cee_del_policy o[4], void * v1, void * v2, 
                              void *v3, void *v4);

struct cee_n_tuple {
  void * _[1];  /* n elements */
};
extern struct cee_n_tuple * cee_n_tuple_mk (struct cee_state * s, size_t n, ...);
extern struct cee_n_tuple * cee_n_tuple_mk_e (struct cee_state * s, size_t n, enum cee_del_policy o[], ...);


struct cee_set {
  void * _;
};

/*
 * a binary tree based set implementation
 * cmp: the function to compare two elements, it returns 0 
 * if they are equal; it returns large than 0 if the first 
 * parameter is larger than the second parameter; it returns 
 * a value smaller than 0 if the first parameter is smaller than
 * the second parameters;
 *
 * dt: specifiy how its element should be handled when the set is deleted.
 *
 */
extern struct cee_set * cee_set_mk (struct cee_state * s, int (*cmp)(const void *, const void *));
extern struct cee_set * cee_set_mk_e (struct cee_state *s, enum cee_del_policy o, 
                                      int (*cmp)(const void *, const void *));

extern void cee_set_add(struct cee_set * m, void * key);
/*
 * if the set is null, return NULL
 */
extern void * cee_set_find(struct cee_set * m, void * key);
/*
 * if the set is null, return NULL
 */
extern void * cee_set_remove(struct cee_set * m, void * key);
extern void cee_set_clear (struct cee_set * m);
/*
 * return the number of elements in the set
 * if the set is null, return 0
 */
extern size_t cee_set_size(struct cee_set * m);
/*
 * return true if the set is null or has no element
 */
extern bool cee_set_empty(struct cee_set * s);
extern struct cee_list * cee_set_values (struct cee_set * m);
extern struct cee_set * cee_set_union_sets (struct cee_set * s1, struct cee_set * s2);

extern void cee_set_iterate (struct cee_set *s, void *ctx,
			     void (*f)(void *ctx, void *value));

struct cee_map {
  void * _;
};

/*
 * map implementation based on binary tree
 * add/remove
 */
extern struct cee_map * cee_map_mk(struct cee_state * s, cee_cmp_fun cmp);
extern struct cee_map * cee_map_mk_e(struct cee_state * s, enum cee_del_policy o[2], cee_cmp_fun cmp);

/*
 * return the number of key/value paris in the map
 * if the map is null, return 0
 */
extern uintptr_t cee_map_size(struct cee_map *);
/*
 * if the key does not exist, add a new key-value pair to the map
 * if the key exist, replace the old value with the new value and return 
 * the old value
 */
extern void* cee_map_add(struct cee_map * m, void * key, void * value);
/*
 * if the map is null, return NULL.
 * if the key is found, return its value.
 * otherwise, return NULL.
 */
extern void * cee_map_find(struct cee_map * m, void * key);

/*
 * replace the old_key with the new key
 */
extern bool cee_map_rename(struct cee_map *m, void *old_key, void *new_key);

/*
 * if the map is null, return NULL
 */
extern void * cee_map_remove(struct cee_map *m, void * key);
extern struct cee_list * cee_map_keys(struct cee_map *m);
extern struct cee_list * cee_map_values(struct cee_map *m);

/*
 * applies f to each (k,v) of the map m with ctx
 * if the map is null, return immediately
 * if f return 0, the process succeeds and the iteration continue
 * if f return none zero, the process fails and the iteration stop
 */
extern int cee_map_iterate(struct cee_map *m, void *ctx, int (*f)(void *ctx, void *key, void *value));

/*
 *
 * merge all k/v pairs from src to dest
 * src will never be changed. dest is modified 
 * if src is not empty. 
 *
 * @param merge decides how to merge two values
 * if merge is NULL, the value from src overwrite the value from dest.
 *
 */
extern void cee_map_merge(struct cee_map *dest, struct cee_map *src,
			  void *ctx, void* (*merge)(void *ctx, void *old, void *new));


/*
 * make a shadow copy of a map
 */
extern struct cee_map* cee_map_clone(struct cee_map *src);


/*
 * dict behaviors like a map with the following properties
 * 
 * 1. fixed size
 * 2. key is char *
 * 3. insertion only
 *
 */
struct cee_dict {
  char _[1];  /* opaque data */
};

/*
 *
 */
extern struct cee_dict * cee_dict_mk (struct cee_state * s, size_t n);
extern struct cee_dict * cee_dict_mk_e (struct cee_state * s, enum cee_del_policy o, size_t n);

extern void cee_dict_add(struct cee_dict * d, char * key, void * value);
extern void * cee_dict_find(struct cee_dict * d, char * key);

/*
 * a stack with a fixed size
 */
struct cee_stack {
  void * _[1];
};
/*
 * create a fixed size stack
 * size: the size of the stack
 * dt: specify how its element should be handled when the stack is deleted.
 */
extern struct cee_stack * cee_stack_mk(struct cee_state *s, size_t n);
extern struct cee_stack * cee_stack_mk_e (struct cee_state *s, enum cee_del_policy o, size_t n);

/*
 * return the element nth element away from the top element
 */
extern void * cee_stack_top(struct cee_stack *, size_t nth);
/*
 * pop out the top element and return it
 */
extern void * cee_stack_pop(struct cee_stack *);
/*
 * push an element to the top of the stack
 */
extern int cee_stack_push(struct cee_stack *, void *);
/*
 * test if the stack is empty
 */
extern bool cee_stack_empty (struct cee_stack *);
/*
 * test if the stack is full
 */
extern bool cee_stack_full (struct cee_stack *);
/*
 * return the size of the stack
 */
extern uintptr_t cee_stack_size (struct cee_stack *);
  
  
/*
 * singleton
 */
struct cee_singleton {
  cee_tag_t  tag;
  uintptr_t val;
};
extern struct cee_singleton * cee_singleton_init(void * s, cee_tag_t tag, uintptr_t val);
#define CEE_SINGLETON_SIZE (sizeof(struct cee_singleton) + sizeof(struct cee_sect))
  
  
enum cee_boxed_primitive_type {
  cee_primitive_f64 = 1,
  cee_primitive_f32,
  cee_primitive_u64,
  cee_primitive_u32,
  cee_primitive_u16,
  cee_primitive_u8,
  cee_primitive_i64,
  cee_primitive_i32,
  cee_primitive_i16,
  cee_primitive_i8
};
union cee_boxed_primitive_value {
  double   f64;
  float    f32;
  uint64_t u64;
  uint32_t u32;
  uint16_t u16;
  uint8_t  u8;
  int64_t  i64;
  int32_t  i32;
  int16_t  i16;
  int8_t   i8;
};

/*
 * boxed primitive value
 */
struct cee_boxed {
  union cee_boxed_primitive_value _;
};

extern struct cee_boxed * cee_boxed_from_double(struct cee_state *, double);
extern struct cee_boxed * cee_boxed_from_float(struct cee_state *, float);

extern struct cee_boxed * cee_boxed_from_u64(struct cee_state *, uint64_t);
extern struct cee_boxed * cee_boxed_from_u32(struct cee_state *, uint32_t);
extern struct cee_boxed * cee_boxed_from_u16(struct cee_state *, uint16_t);
extern struct cee_boxed * cee_boxed_from_u8(struct cee_state *, uint8_t);

extern struct cee_boxed * cee_boxed_from_i64(struct cee_state *, int64_t);
extern struct cee_boxed * cee_boxed_from_i32(struct cee_state *, int32_t);
extern struct cee_boxed * cee_boxed_from_i16(struct cee_state *, int16_t);
extern struct cee_boxed * cee_boxed_from_i8(struct cee_state *, int8_t);

extern double   cee_boxed_to_double(struct cee_boxed * x);
extern float    cee_boxed_to_float(struct cee_boxed * x);

extern uint64_t cee_boxed_to_u64(struct cee_boxed * x);
extern uint32_t cee_boxed_to_u32(struct cee_boxed * x);
extern uint16_t cee_boxed_to_u16(struct cee_boxed * x);
extern uint8_t  cee_boxed_to_u8(struct cee_boxed * x);

extern int64_t  cee_boxed_to_i64(struct cee_boxed * x);
extern int32_t  cee_boxed_to_i32(struct cee_boxed * x);
extern int16_t  cee_boxed_to_i16(struct cee_boxed * x);
extern int8_t   cee_boxed_to_i8(struct cee_boxed * x);

/*
 * number of bytes needed to print out the value
 */
extern size_t cee_boxed_snprint(char * buf, size_t size, struct cee_boxed *p);
  
union cee_tagged_ptr {
  void * _;
  struct cee_str       * str;
  struct cee_set       * set;
  struct cee_list      * list;
  struct cee_map       * map;
  struct cee_dict      * dict;
  struct cee_tuple     * tuple;
  struct cee_triple    * triple;
  struct cee_quadruple * quadruple;
  struct cee_block     * block;
  struct cee_boxed     * boxed;
  struct cee_singleton * singleton;
  struct cee_stack     * stack;
  struct cee_tagged    * tagged;
};


/*
 * the generic tagged value is useful to construct tagged union
 * runtime checking is needed. 
 */
struct cee_tagged {
  cee_tag_t tag;
  union cee_tagged_ptr ptr;
};

/*
 * tag: any integer value
 * v: a pointer
 */
extern struct cee_tagged * cee_tagged_mk (struct cee_state *, uintptr_t tag, void * v);
extern struct cee_tagged * cee_tagged_mk_e (struct cee_state *, enum cee_del_policy o, uintptr_t tag, void *v);

struct cee_env {
  struct cee_env  * outer;
  struct cee_map  * vars;
};
extern struct cee_env * cee_env_mk(struct cee_state *, struct cee_env * outer, struct cee_map *vars);
extern struct cee_env * cee_env_mk_e(struct cee_state *, enum cee_del_policy dp[2], struct cee_env * outer, struct cee_map * vars);
extern void * cee_env_find(struct cee_env * e, char * key);

struct cee_closure {
  struct cee_env * env;
  void * (*vfun)(struct cee_state * s, struct cee_env * env, size_t n, va_list ap);
};

extern struct cee_closure * cee_closure_mk (struct cee_state * s, struct cee_env * env, void * fun);
extern void * cee_closure_call (struct cee_state * s, struct cee_closure * c, size_t n, ...);

extern void cee_use_realloc(void *);
extern void cee_use_malloc(void *);
  
  /*
   * release the memory block pointed by p immediately
   * it may follow the points-to edges to delete
   *    the in-degree (reference count) of targeted memory blocks
   *    or targeted memory blocks
   *
   */
extern void cee_del (void *);
extern void cee_del_ref(void *);
extern void cee_del_e (enum cee_del_policy o, void * p);

extern void cee_trace (void *p, enum cee_trace_action ta);
extern int cee_cmp (void *, void *);

extern void cee_incr_indegree (enum cee_del_policy o, void * p);
extern void cee_decr_indegree (enum cee_del_policy o, void * p);

/* 
 * return the reference count of an object
 */
extern uint16_t cee_get_rc (void *);

/*
 * call this to cause segfault for non-recoverable errors
 */
extern void cee_segfault() __attribute__((noreturn));

struct cee_state {
  struct cee_stack * stack;  /* the stack */
  /* arbitrary number of contexts */
  /* all memory blocks are reachables from the roots */
  /* are considered alive in gc */
  struct cee_map   * contexts; /* TODO: should be a stack of contexts to support nested lexical scope */


  struct cee_set   * roots; 

  /* the mark value for the next mark/sweep iteration */
  int                next_mark;

  /* all memory blocks ever allocated with this state.
   * it is used to find them to free in the invocation 
   * of cee_del(cee_state *)
   */
  struct cee_sect  * trace_tail;
};
/*
 * @param n:the size of stack, which is used for parsing
 * json and js
 */
extern struct cee_state * cee_state_mk(size_t n);
/*
 * add a cee_* pointer to the gc root so it can be
 * reachable from the root, and survive the next 
 * cee_state_gc call
 */
extern void cee_state_add_gc_root(struct cee_state *, void *);
/*
 * remove a cee_* pointer from the gc root so it will
 * not be reachable from the root, and be collected (freed)
 * by the next cee_state_gc call
 */
extern void cee_state_remove_gc_root(struct cee_state *, void *);
extern void cee_state_gc(struct cee_state *);
extern void cee_state_add_context(struct cee_state *, char * key, void * val);
extern void cee_state_remove_context(struct cee_state *, char * key);
extern void * cee_state_get_context(struct cee_state *, char * key);

extern struct cee_state* cee_get_state(void *p);

#ifndef NULL
#define NULL     ((void *)0)
#endif

#endif /* CEE_H */
