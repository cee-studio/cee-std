/* JSON snprint
   C reimplementation of cppcms's json.cpp
*/
#ifndef CEE_JSON_AMALGAMATION
#include "cee-json.h"
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#endif

struct counter {
  uintptr_t next;
  struct cee_list * array;
  struct cee_map  * object;
  char tabs;
  char more_siblings;
};

static struct counter * push(struct cee_state * st, uintptr_t tabs, bool more_siblings,
                             struct cee_stack * sp, struct cee_json * j) {
  struct counter * p = NULL;
  if (j == NULL) {
    p = cee_block_mk(st, sizeof(struct counter));
    p->tabs = 0;
  }
  else {
    switch(j->t) {
      case CEE_JSON_OBJECT:
        {
          p = cee_block_mk(st, sizeof(struct counter));
          struct cee_map * mp = cee_json_to_object(j);
          p->array = cee_map_keys(mp);
          p->object = cee_json_to_object(j);
          p->tabs = tabs;
          p->next = 0;
          p->more_siblings = 0;
        }
        break;
      case CEE_JSON_ARRAY:
        {
          p = cee_block_mk(st, sizeof(struct counter));
          p->array = cee_json_to_array(j);
          p->tabs = tabs;
          p->next = 0;
          p->more_siblings = 0;
        }
        break;
      default:  
        {
          p = cee_block_mk(st, sizeof(struct counter));
          p->array = NULL;
          p->tabs = tabs;
          p->next = 0;
          p->more_siblings = 0;
        }
        break;
    }
    p->more_siblings = more_siblings;
  }
  enum cee_del_policy o[2] = { CEE_DP_DEL, CEE_DP_NOOP };
  cee_stack_push(sp, cee_tuple_mk_e(st, o, p, j));
  return p;
}

static void pad (uintptr_t * offp, char * buf, struct counter * cnt, 
                 enum cee_json_fmt f) {
  if (!f) return;
  
  uintptr_t offset = *offp;
  if (buf) {
    int i;
    for (i = 0; i < cnt->tabs; i++)
      buf[offset + i] = '\t';
  }
  offset += cnt->tabs;
  *offp = offset;
  return;
}

static void delimiter (uintptr_t * offp, char * buf, enum cee_json_fmt f, 
                       struct counter * cnt, char c) 
{
  uintptr_t offset = *offp;
  if (!f) {
    if (buf) buf[offset] = c;
    offset ++; /*  only count one */
    *offp = offset;
    return;
  }
  
  switch (c) {
    case '[':
    case '{':
      pad(offp, buf, cnt, f);
      if (buf) {
        buf[offset] = c;
        buf[offset+1] = '\n';
      }
      offset +=2;
      break;
    case ']':
    case '}':
      if (buf) buf[offset] = '\n';
      offset ++;
      pad(&offset, buf, cnt, f);
      if (buf) buf[offset] = c;
      offset ++;
      if (buf) buf[offset] = '\n';
      offset ++;
      break;
    case ':':
      if (buf) {
        buf[offset] = ' ';
        buf[offset+1] = ':';
        buf[offset+2] = '\t';
      }
      offset +=3;
      break;
    case ',':
      if (buf) {
        buf[offset] = ',';
        buf[offset+1] = '\n';
      }
      offset +=2;
      break;
  }
  *offp = offset;
}


static void str_append(char * out, uintptr_t *offp, char *begin, unsigned len) {
  uintptr_t offset = *offp;
  
  if (out) out[offset] = '"';
  offset ++;
  
  char *i,*last;
  char buf[8] = "\\u00";
  for(i=begin,last = begin;i < begin + len;) {
    char *addon = 0;
    unsigned char c=*i;
    switch(c) {
    case 0x22: addon = "\\\""; break;
    case 0x5C: addon = "\\\\"; break;
    case '\b': addon = "\\b"; break;
    case '\f': addon = "\\f"; break;
    case '\n': addon = "\\n"; break;
    case '\r': addon = "\\r"; break;
    case '\t': addon = "\\t"; break;
    default:
      if(c<=0x1F) {
        static char const tohex[]="0123456789abcdef";
        buf[4]=tohex[c >>  4];
        buf[5]=tohex[c & 0xF];
        buf[6]=0;
        addon = buf;
      }
    };
    if(addon) {
      /* a.append(last,i-last); */
      if (out) memcpy(out+offset, last, i-last);
      offset += i-last;
      
      if (out) memcpy(out+offset, addon, strlen(addon));
      offset += strlen(addon);
      i++;
      last = i;
    }
    else {
      i++;
    }
  }
  if (out) memcpy(out+offset, last, i-last);
  offset += i-last;
  if (out) out[offset] = '"';
  offset++;
  *offp = offset;
}

/*
 * compute how many bytes are needed to serialize cee_json as a string
 */
ssize_t cee_json_snprint (struct cee_state *st, char *buf, size_t size, struct cee_json *j,
			  enum cee_json_fmt f) {
  struct cee_tuple * cur;
  struct cee_json * cur_json;
  struct counter * ccnt;
  uintptr_t incr = 0;
  
  struct cee_stack *sp = cee_stack_mk_e(st, CEE_DP_NOOP, 500);
  push (st, 0, false, sp, j);
  
  uintptr_t offset = 0;
  while (!cee_stack_empty(sp) && !cee_stack_full(sp)) {
    cur = cee_stack_top(sp, 0);
    cur_json = (struct cee_json *)(cur->_[1]);
    ccnt = (struct counter *)(cur->_[0]);
    
    switch(cur_json->t) {
      case CEE_JSON_NULL: 
        {
          pad(&offset, buf, ccnt, f);
          if (buf)
            memcpy(buf + offset, "null", 4);
          offset += 4;
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_BOOLEAN: 
        {
          pad(&offset, buf, ccnt, f);
          char * s = "false";
          bool b;
          cee_json_to_bool(cur_json, &b);
          if (b)
            s = "true";
          if (buf) 
            memcpy(buf + offset, s, strlen(s));
          offset += strlen(s);
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_UNDEFINED:
        {
          pad(&offset, buf, ccnt, f);
          if (buf)
            memcpy(buf + offset, "undefined", 9);
          offset += 9;
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_STRING:
        {
          char *str = (char *)cee_json_to_str(cur_json);
          pad(&offset, buf, ccnt, f);
          /* TODO: escape str */
          str_append(buf, &offset, str, strlen(str));
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_BLOB:
        {
          /* TODO: encode as base64 */
          char *str = "error:blob is not handled in print";
          pad(&offset, buf, ccnt, f);
          str_append(buf, &offset, str, strlen(str));
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_DOUBLE:
      case CEE_JSON_I64:
      case CEE_JSON_U64:
        {
          pad(&offset, buf, ccnt, f);
          incr = cee_boxed_snprint (NULL, 0, cee_json_to_boxed(cur_json));
          if (buf)
            cee_boxed_snprint (buf+offset, incr+1/*\0*/, cee_json_to_boxed(cur_json));
          offset+=incr;
          if (ccnt->more_siblings)
            delimiter(&offset, buf, f, ccnt, ',');
          cee_del(cee_stack_pop(sp));
        }
        break;
      case CEE_JSON_ARRAY: 
        { 
          uintptr_t i = ccnt->next;
          if (i == 0) 
            delimiter(&offset, buf, f, ccnt, '[');
          
          uintptr_t n = cee_list_size(ccnt->array);
          if (i < n) {
            bool more_siblings = false;
            if (1 < n && i+1 < n)
              more_siblings = true;
            ccnt->next++;
            push (st, ccnt->tabs + 1, more_siblings, sp, 
                  (struct cee_json *)(ccnt->array->a[i]));
          } 
          else {
            delimiter(&offset, buf, f, ccnt, ']');
            if (ccnt->more_siblings)
              delimiter(&offset, buf, f, ccnt, ',');
            cee_del(cee_stack_pop(sp));
          }
        }
        break;
      case CEE_JSON_OBJECT:
        {
          uintptr_t i = ccnt->next;
          if (i == 0)
            delimiter(&offset, buf, f, ccnt, '{');
          
          uintptr_t n = cee_list_size(ccnt->array);
          if (i < n) {
            bool more_siblings = false;
            if (1 < n && i+1 < n)
              more_siblings = true;

            ccnt->next++;
            char * key = (char *)ccnt->array->a[i];
            struct cee_json * j1 = cee_map_find(ccnt->object, ccnt->array->a[i]);
            unsigned klen = strlen(key);
            pad(&offset, buf, ccnt, f);
            str_append(buf, &offset, key, klen);
            delimiter(&offset, buf, f, ccnt, ':');
            push(st, ccnt->tabs + 1, more_siblings, sp, j1);
          }
          else {
            delimiter(&offset, buf, f, ccnt, '}');
            if (ccnt->more_siblings)
              delimiter(&offset, buf, f, ccnt, ',');
            cee_del(ccnt->array);
            cee_del(cee_stack_pop(sp));
          }
        }
        break;
    }
  }
  cee_del (sp);
  if (buf)
    buf[offset] = '\0';
  return offset;
}


/*
 * the memory allocated by this function is tracked by cee_state
 */
ssize_t
cee_json_asprint(struct cee_state *st, char **buf_p, size_t *buf_size_p, struct cee_json *j, 
                 enum cee_json_fmt f)
{
  size_t buf_size = cee_json_snprint(st, NULL, 0, j, f) + 1/*\0*/;
  char *buf = cee_block_mk(st, buf_size);
  if (buf_size_p) *buf_size_p = buf_size;
  *buf_p = buf;
  return cee_json_snprint(st, buf, buf_size, j, f);
}
