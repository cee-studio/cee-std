/*
 * add a struct to the trace chain of st so if cee_del is called on
 * st, this struct is freed too.
 */
static void S(chain) (struct S(header) * h, struct cee_state * st) {
  h->cs.state = st;

  h->cs.trace_prev = st->trace_tail;
  st->trace_tail->trace_next = &h->cs;

  st->trace_tail = &h->cs;
}


/*
 * remove a struct to the trace chain, this should
 * be called whenver the struct is to be freed.
 */
static void S(de_chain) (struct S(header) * h) {
  struct cee_state * st = h->cs.state;

  struct cee_sect * prev = h->cs.trace_prev;
  struct cee_sect * next = h->cs.trace_next;

  if (st->trace_tail == &h->cs) {
    prev->trace_next = NULL;
    st->trace_tail = prev;
  }
  else {
    prev->trace_next = next;
    if (next)
      next->trace_prev = prev;
  }
}

/*
 * resize a struct, and update its existance in the trace chain
 */
static struct S(header) * S(resize)(struct S(header) * h, size_t n) 
{
  struct cee_state * state = h->cs.state;
  struct S(header) * ret;
  switch(h->cs.resize_method)
  {
    case CEE_RESIZE_WITH_REALLOC:
      S(de_chain)(h); /* remove the old struct from the chain */
      ret = realloc(h, n);
      ret->cs.mem_block_size = n;
      S(chain)(ret, state); /* add the new struct to the chain */
      break;
    case CEE_RESIZE_WITH_MALLOC: 
      /* TODO: remove this option, it errors on correctness at the cost of leaking memory */
      ret = malloc(n); 
      memcpy(ret, h, h->cs.mem_block_size);
      ret->cs.mem_block_size = n;
      S(chain)(ret, state); /* add the new struct to the chain */
      break;
    case CEE_RESIZE_WITH_IDENTITY:
      ret = h;
      break;
  }
  return ret;
}
