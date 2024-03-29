/* cee_json parser
   C reimplementation of cppcms's json.cpp
*/
#ifndef CEE_JSON_AMALGAMATION
#include "cee-json.h"
#include "cee.h"
#include "tokenizer.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#endif

enum state_type {
  st_init = 0,
  st_object_or_array_or_value_expected = 0 ,
  st_object_key_or_close_expected,
  st_object_colon_expected,
  st_object_value_expected,
  st_object_close_or_comma_expected,
  st_array_value_or_close_expected,
  st_array_close_or_comma_expected,
  st_error,
  st_done
} state_type;


static const uintptr_t cee_json_max_depth = 512;

#define SPI(st, state, j)  cee_tuple_mk_e(st, (enum cee_del_policy [2]){CEE_DP_NOOP, CEE_DP_NOOP}, (void *)state, j)
#define ARR_LEN 20

int cee_json_parsex(struct cee_state * st, char * buf, uintptr_t len, struct cee_json **out, bool force_eof,
		    int *error_at_line)
{
  struct tokenizer tock = {0};
  tock.buf = buf;
  tock.buf_end = buf + len;
  *out = NULL;
  
  enum state_type state = st_init;
  struct cee_str * key = NULL;
  
  struct cee_stack * sp = cee_stack_mk_e(st, CEE_DP_NOOP, cee_json_max_depth);
  struct cee_tuple * top = NULL;
  struct cee_tuple * result = NULL;

#define TOPS (enum state_type)(top->_[0])
#define POP(sp) \
  do { result = (struct cee_tuple *)cee_stack_pop(sp); } while(0)
  
  cee_stack_push(sp, SPI(st, st_done, NULL));

  while(!cee_stack_empty(sp) && !cee_stack_full(sp)
          && state != st_error && state != st_done) 
  {
    if (result) {
      cee_del(result);
      result = NULL;
    }

    int c = cee_json_next_token(st, &tock);
#if 0
    fprintf(stderr, "token %c\n", c);
#endif
    
    top = (struct cee_tuple *)cee_stack_top(sp, 0);
    switch(state) {
    case st_object_or_array_or_value_expected:
        if(c=='[')  {
          top->_[1]=cee_json_array_mk(st, ARR_LEN);
          state=st_array_value_or_close_expected;
        }
        else if(c=='{') {
          top->_[1]=cee_json_object_mk(st);
          state=st_object_key_or_close_expected;
        }
        else if(c==tock_str)  {
          top->_[1]=cee_json_str_mk(st, tock.str);
          tock.str = NULL;
          state=TOPS;
          POP(sp);
        }
        else if(c==tock_true) {
          top->_[1]=cee_json_true();
          state=TOPS;
          POP(sp);
        }
        else if(c==tock_false) {
          top->_[1]=cee_json_false();
          state=TOPS;
          POP(sp);
        }
        else if(c==tock_null) {
          top->_[1]=cee_json_null();
          state=TOPS;
          POP(sp);
        }
        else if(c==tock_number) {
          if (tock.type == NUMBER_IS_I64)
            top->_[1] = cee_json_i64_mk(st, tock.number.i64);
          else if (tock.type == NUMBER_IS_U64)
            top->_[1] = cee_json_u64_mk(st, tock.number.u64);
          else
            top->_[1] = cee_json_double_mk(st, tock.number.real);
          state=TOPS;
          POP(sp);
        }
        else
          state = st_error;
        break;
    case st_object_key_or_close_expected:
        if(c=='}') {
          state=TOPS;
          POP(sp);
        } 
        else if (c==tock_str) {
          key = tock.str;
          tock.str = NULL;
          state = st_object_colon_expected;
        }
        else
          state = st_error;
        break;
    case st_object_colon_expected:
        if(c!=':')
          state=st_error;
        else
          state=st_object_value_expected;
        break;
    case st_object_value_expected: {
        struct cee_map * obj = cee_json_to_object(top->_[1]);
        if(c==tock_str) {         
          cee_map_add(obj, key, cee_json_str_mk(st, tock.str));
          tock.str = NULL;
          state=st_object_close_or_comma_expected;
        }
        else if(c==tock_true) {
          cee_map_add(obj, key, cee_json_true());
          state=st_object_close_or_comma_expected;
        }
        else if(c==tock_false) {
          cee_map_add(obj, key, cee_json_false());
          state=st_object_close_or_comma_expected;
        }
        else if(c==tock_null) {
          cee_map_add(obj, key, cee_json_null());
          state=st_object_close_or_comma_expected;
        }
        else if(c==tock_number) {
          if (tock.type == NUMBER_IS_I64)
            cee_map_add(obj, key, cee_json_i64_mk(st, tock.number.i64));
          else if (tock.type == NUMBER_IS_U64)
            cee_map_add(obj, key, cee_json_u64_mk(st, tock.number.u64));
          else
            cee_map_add(obj, key, cee_json_double_mk(st, tock.number.real));
          state=st_object_close_or_comma_expected;
        }
        else if(c=='[') {
          struct cee_json * a = cee_json_array_mk(st, ARR_LEN);
          cee_map_add(obj, key, a);
          state=st_array_value_or_close_expected;
          cee_stack_push(sp, SPI(st, st_object_close_or_comma_expected, a));
        }
        else if(c=='{') {
          struct cee_json * o = cee_json_object_mk(st);
          cee_map_add(obj, key, o);
          state=st_object_key_or_close_expected;
          cee_stack_push(sp, SPI(st, st_object_close_or_comma_expected, o));
        }
        else 
          state=st_error;
        break; }
    case st_object_close_or_comma_expected:
        if(c==',')
          state=st_object_key_or_close_expected;
        else if(c=='}') {
          state=TOPS;
          POP(sp);
        }
        else
          state=st_error;
        break;
    case st_array_value_or_close_expected: {
        if(c==']') {
          state=TOPS;
          POP(sp);
          break;
        }
        struct cee_list * ar = cee_json_to_array(top->_[1]);
        
        if(c==tock_str) {
          cee_list_append(ar, cee_json_str_mk(st, tock.str));
          state=st_array_close_or_comma_expected;
        } 
        else if(c==tock_true) {
          cee_list_append(ar, cee_json_true());
          state=st_array_close_or_comma_expected;
        } 
        else if(c==tock_false) {
          cee_list_append(ar, cee_json_false());
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_null) {
          cee_list_append(ar, cee_json_null());
          state=st_array_close_or_comma_expected;
        }
        else if(c==tock_number) {
          if (tock.type == NUMBER_IS_I64)
            cee_list_append(ar, cee_json_i64_mk(st, tock.number.i64));
          else if (tock.type == NUMBER_IS_U64)
            cee_list_append(ar, cee_json_u64_mk(st, tock.number.u64));
          else
            cee_list_append(ar, cee_json_double_mk(st, tock.number.real));
          state=st_array_close_or_comma_expected;
        }
        else if(c=='[') {
          struct cee_json * a = cee_json_array_mk(st, ARR_LEN);
          cee_list_append(ar, a);
          state=st_array_value_or_close_expected;
          cee_stack_push(sp, SPI(st, st_array_close_or_comma_expected,a));
        }
        else if(c=='{') {
          struct cee_json * o = cee_json_object_mk(st);
          cee_list_append(ar, o);
          state=st_object_key_or_close_expected;
          cee_stack_push(sp, SPI(st, st_array_close_or_comma_expected,o));
        }
        else 
          state=st_error;
        break; }
    case st_array_close_or_comma_expected:
        if(c==']') {
          state=TOPS;
          POP(sp);
        }
        else if(c==',')
          state=st_array_value_or_close_expected;
        else
          state=st_error;
        break;
    case st_done:
    case st_error:
        break;
    };
  }
  
  cee_del(sp);
  if(state==st_done) { 
    if(force_eof) {
      if(cee_json_next_token(st, &tock)!=tock_eof) {
        *error_at_line=tock.line;
        return EINVAL;
      }
    }
    *out = (struct cee_json *)(result->_[1]);
    cee_del(result);
    return 0;
  }
  *error_at_line=tock.line;
  return EINVAL;
}
