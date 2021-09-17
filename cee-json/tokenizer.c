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

static bool read_4_digits(char ** str_p, char * const buf_end, uint16_t *x)
{
  char * str = * str_p;
  if (buf_end - str < 4)
    return false;

  char buf[5] = { 0 };
  int i;
  for(i=0; i<4; i++) {
    char c=str[i];
    buf[i] = c;
    if( ('0'<= c && c<='9') || ('A'<= c && c<='F') || ('a'<= c && c<='f') )
      continue;

    return false;
  }
  unsigned v;
  sscanf(buf,"%x",&v);
  *x=v;
  *str_p = str + 4;
  return true;
}

static int utf16_is_first_surrogate(uint16_t x)
{
  return 0xD800 <=x && x<= 0xDBFF;
}

static int utf16_is_second_surrogate(uint16_t x)
{
  return 0xDC00 <=x && x<= 0xDFFF;
}

static uint32_t utf16_combine_surrogate(uint16_t w1,uint16_t w2)
{
  return ((((uint32_t)w1 & 0x3FF) << 10) | (w2 & 0x3FF)) + 0x10000;
}

static void * append (uint32_t x, char *d)
{
  struct utf8_seq seq = { {0}, 0 };
  utf8_encode(x, &seq);
  for (unsigned i = 0; i < seq.len; ++i, d++)
    *d = seq.c[i];
  return d;
}

int
json_string_unescape(char **output_p, size_t *output_len_p,
                     char *input, size_t input_len)
{
  unsigned char c;
  char * const input_start = input, * const input_end = input + input_len;
  char * out_start = NULL, * d = NULL, * s = NULL;
  uint16_t first_surrogate;
  int second_surrogate_expected;


  enum state {
    TESTING = 1,
    ALLOCATING,
    UNESCAPING,
  } state = TESTING;

second_iter:
  first_surrogate = 0;
  second_surrogate_expected = 0;
  for (s = input_start; s < input_end;) {
    c = * s;
    s ++;

    if (second_surrogate_expected && c != '\\')
      goto return_err;

    if (0<= c && c <= 0x1F)
      goto return_err;

    if('\\' == c) {
      if (TESTING == state) {
        state = ALLOCATING;
        break; // break the while loop
      }

      if (s == input_end) {
        //input is not a well-formed json string
        goto return_err;
      }

      c = * s;
      s ++;

      if (second_surrogate_expected && c != 'u')
        goto return_err;

      switch(c) {
      case	'"':
      case	'\\':
      case	'/':
          *d = c; d++; break;
      case	'b': *d = '\b'; d ++;  break;
      case	'f': *d = '\f'; d ++;  break;
      case	'n': *d = '\n'; d ++;  break;
      case	'r': *d = '\r'; d ++;  break;
      case	't': *d = '\t'; d ++;  break;
      case	'u':
        {
          uint16_t x;
          if (!read_4_digits(&s, input_end, &x))
            goto return_err;
          if (second_surrogate_expected) {
            if (!utf16_is_second_surrogate(x))
              goto return_err;
            d = append(utf16_combine_surrogate(first_surrogate, x), d);
            second_surrogate_expected = 0;
          } else if (utf16_is_first_surrogate(x)) {
            second_surrogate_expected = 1;
            first_surrogate = x;
          } else {
            d = append(x, d);
          }
          break;
        }
      default:
          if(0<= c && c <= 0x1F) /* report errors */
            goto return_err;
      }
    }
    else if (UNESCAPING == state) {
      *d = c;
      d++;
    }
  }

  switch (state) {
  case UNESCAPING:
      if (!utf8_validate(out_start, d))
        goto return_err;
      else
      {
        *output_p = out_start;
        *output_len_p = d - out_start;
        return 1;
      }
  case ALLOCATING:
      out_start = calloc(1, input_len);
      d = out_start;
      state = UNESCAPING;
      goto second_iter;
  case TESTING:
      *output_p = input_start;
      *output_len_p = input_len;
      return 1;
  default:
      break;
  }

return_err:
  if (UNESCAPING == state)
    free(out_start);
  return 0;
}

static bool parse_string(struct cee_state * st, struct tokenizer * t) {
  char *start = t->buf + 1; // start after initial '"'
  char *end = start;

  // reach the end of the string
  while (*end != '\0' && *end != '\"') {
    if ('\\' == *end++ && *end != '\0') { // check for escaped characters
      ++end; // eat-up escaped character
    }
  }
  if (*end != '\"') return false; // make sure reach end of string

  char * unscp_str = NULL;
  size_t unscp_len = 0;
  if (json_string_unescape(&unscp_str,  &unscp_len, start, end-start)) {
    if (unscp_str == start) {
      t->str = cee_str_mk_e(st, end-start, "%s", start);
    }
    else {
      /// @todo? create a cee_str func that takes ownership of string
      t->str = cee_str_mk_e(st, unscp_len, "%s", unscp_str);
      free(unscp_str);
    }
    t->buf = end + 1; // '"' + 1
    return true;
  }
  return false; // ill formed string
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
    case '\"':
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
