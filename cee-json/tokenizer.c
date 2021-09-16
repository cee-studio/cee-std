#ifndef CEE_JSON_AMALGAMATION
#include <stdlib.h>
#include "cee.h"
#include "cee-json.h"
#include "utf8.h"
#include "tokenizer.h"
#endif

#include <ctype.h>

static bool check(char * buf, char * s, char **ret)
{
  char * next = buf;
  
  for (next = buf; *s && *next == *s; next++, s++);
  if (*s==0) {
    *ret = next;
    return true;
  }
  else {
    *ret = buf;
  	return false;
  }
  return false;
}

static bool read_4_digits(struct tokenizer * t, uint16_t *x)
{
  char *buf;
  if (t->buf_end - t->buf >= 5) {
    buf = t->buf;
  } 
  else
    return false;
  int i;
  for(i=0; i<4; i++) {
    char c=buf[i];
    if(	('0'<= c && c<='9') || ('A'<= c && c<='F') || ('a'<= c && c<='f') ) {
      continue;
    }
    return false;
  }
  unsigned v;
  sscanf(buf,"%x",&v);
  *x=v;
  return true;
}

static bool parse_string(struct cee_state * st, struct tokenizer * t) {
  char c;
  // we should use a more efficient stretchy buffer here
  t->str = cee_str_mk_e(st, 128, "");
  
  if (t->buf == t->buf_end)
    return false;
  c=t->buf[0];
  t->buf++;
  
  if (c != '"') return false;
  bool second_surrogate_expected=false;
  uint16_t first_surrogate = 0;
  
  for(;;) {
    if(t->buf == t->buf_end)
      return false;
    c = t->buf[0];
    t->buf ++;
    
    if(second_surrogate_expected && c!='\\')
      return false;
    if(0<= c && c <= 0x1F)
      return false;
    if(c=='"')
      break;
    if(c=='\\') {
      if(t->buf == t->buf_end)
        return false;
      if(second_surrogate_expected && c!='u')
        return false;
      switch(c) {
      case	'"':
      case	'\\':
      case	'/':
        t->str = cee_str_add(t->str, c);
        break;
      case	'b': t->str = cee_str_add(t->str, '\b'); break;
      case	'f': t->str = cee_str_add(t->str, '\f'); break;
      case	'n': t->str = cee_str_add(t->str, '\n'); break;
      case	'r': t->str = cee_str_add(t->str, '\r'); break; 
      case	't': t->str = cee_str_add(t->str, '\t'); break;
      case	'u': 
        {
          // don't support utf16
          uint16_t x;
          if (!read_4_digits(t, &x)) 
            return false;
        	struct utf8_seq s = { 0 };
          utf8_encode(x, &s);
          t->str = cee_str_ncat(t->str, s.c, s.len);
        }
        break;
      default:
        return false;
      }
    }
    else {
      t->str = cee_str_add(t->str, c);
    }
  }
  if(!utf8_validate(t->str->_, cee_str_end(t->str)))
    return false;
  return true;
}


static bool parse_number(struct tokenizer *t) {
  char *start = t->buf;
  char *end = start;

  /* 1st STEP: check for a minus sign and skip it */
  if ('-' == *end) 
    ++end;
  if (!isdigit(*end)) 
    return false;

  /* 2nd STEP: skips until a non digit char found */
  while (isdigit(*++end)) 
    continue;

  /* 3rd STEP: if non-digit char is not a comma then it must be
      an integer*/
  if ('.' == *end) {
    while (isdigit(*++end)) 
      continue;
  }

  /* 4th STEP: if exponent found skips its tokens */
  if ('e' == *end || 'E' == *end) {
    ++end;
    if ('+' == *end || '-' == *end) { 
      ++end;
    }
    if (!isdigit(*end)) 
      return false;
    while (isdigit(*++end))
      continue;
  }

  /* 5th STEP: convert string to double and return its value */
  char numstr[32];
  snprintf(numstr, sizeof(numstr), "%.*s", (int)(end - start), start);

  t->buf = end; /* skips entire length of number */

  return 1 == sscanf(numstr, "%lf", &t->real);
}

enum token cee_json_next_token(struct cee_state * st, struct tokenizer * t) {
  for (;;t->buf++) {
    if (t->buf >= t->buf_end)
      return tock_eof;
    char c = t->buf[0];
    switch (c) {
      case '[':
      case '{':
      case ':':
      case ',':
      case '}':
      case ']':
        t->buf++;
        return c;
      case '\n':
      case '\r':
        t->line++;
      /* fallthrough */
      case ' ':
      case '\t':
        break;
      case '"':
        if(parse_string(st, t))
          return tock_str;
        return tock_err;
      case 't':
        t->buf++;
        if(check(t->buf, "rue", &t->buf))
          return tock_true;
        return tock_err;
      case 'n':
        t->buf++;
        if(check(t->buf, "ull", &t->buf))
          return tock_null;
        return tock_err;
      case 'f':
        t->buf++;
        if(check(t->buf, "alse", &t->buf))
          return tock_false;
        return tock_err;
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if(parse_number(t))
          return tock_number;
        return tock_err;
      case '/':
        t->buf++;
        if(check(t->buf + 1, "/", &t->buf)) {
          for (;t->buf < t->buf_end && (c = t->buf[0]) && c != '\n'; t->buf++);

          if(c=='\n')
            break;
          return tock_eof;
        }
        return tock_err;
      default:
        t->buf++;
        return tock_err;
    }
  }
}
