#ifndef CEE_JSON_AMALGAMATION
#include <stdlib.h>
#include <ctype.h>
#include "cee.h"
#include "cee-json.h"
#include "tokenizer.h"
#endif

static const uint32_t utf_illegal = 0xFFFFFFFFu;
static bool utf_valid(uint32_t v)
{
  if(v>0x10FFFF)
    return false;
  if(0xD800 <=v && v<= 0xDFFF) /*  surragates */
    return false;
  return true;
}

/* namespace utf8 { */
static bool utf8_is_trail(char ci)
{
  unsigned char c=ci;
  return (c & 0xC0)==0x80;
}


static int utf8_trail_length(unsigned char c) 
{
  if(c < 128)
    return 0;
  if(c < 194)
    return -1;
  if(c < 224)
    return 1;
  if(c < 240)
    return 2;
  if(c <=244)
    return 3;
  return -1;
}

static int utf8_width(uint32_t value)
{
  if(value <=0x7F) {
    return 1;
  }
  else if(value <=0x7FF) {
    return 2;
  }
  else if(value <=0xFFFF) {
    return 3;
  }
  else {
    return 4;
  }
}

/*  See RFC 3629 */
/*  Based on: http://www.w3.org/International/questions/qa-forms-utf-8 */
static uint32_t next(char ** p, char * e, bool html)
{
  if(*p==e)
    return utf_illegal;

  unsigned char lead = **p;
  (*p)++;

  /*  First byte is fully validated here */
  int trail_size = utf8_trail_length(lead);

  if(trail_size < 0)
    return utf_illegal;

  /*  */
  /*  Ok as only ASCII may be of size = 0 */
  /*  also optimize for ASCII text */
  /*  */
  if(trail_size == 0) {
    if(!html || (lead >= 0x20 && lead!=0x7F) || lead==0x9 || lead==0x0A || lead==0x0D)
      return lead;
    return utf_illegal;
  }

  uint32_t c = lead & ((1<<(6-trail_size))-1);

  /*  Read the rest */
  unsigned char tmp;
  switch(trail_size) {
    case 3:
      if(*p==e)
        return utf_illegal;
      tmp = **p;
      (*p)++;
      if (!utf8_is_trail(tmp))
        return utf_illegal;
      c = (c << 6) | ( tmp & 0x3F);
    case 2:
      if(*p==e)
        return utf_illegal;
      tmp = **p;
      (*p)++;
      if (!utf8_is_trail(tmp))
        return utf_illegal;
      c = (c << 6) | ( tmp & 0x3F);
    case 1:
      if(*p==e)
        return utf_illegal;
      tmp = **p;
      (*p)++;
      if (!utf8_is_trail(tmp))
        return utf_illegal;
      c = (c << 6) | ( tmp & 0x3F);
  }

  /*  Check code point validity: no surrogates and */
  /*  valid range */
  if(!utf_valid(c))
    return utf_illegal;

  /*  make sure it is the most compact representation */
  if(utf8_width(c)!=trail_size + 1)
    return utf_illegal;

  if(html && c<0xA0)
    return utf_illegal;
  return c;
} /*  valid */


/*
bool validate_with_count(char * p, char * e, size_t *count,bool html)
{
  while(p!=e) {
    if(next(p,e,html)==utf_illegal)
      return false;
    (*count)++;
  }
  return true;
}
*/

static bool utf8_validate(char * p, char * e)
{
  while(p!=e) 
    if(next(&p, e, false)==utf_illegal)
      return false;
  return true;
}


struct utf8_seq {
  char c[4];
  unsigned len;
};

static void utf8_encode(uint32_t value, struct utf8_seq *out) {
  /* struct utf8_seq out={0}; */
  if(value <=0x7F) {
    out->c[0]=value;
    out->len=1;
  }
  else if(value <=0x7FF) {
    out->c[0]=(value >> 6) | 0xC0;
    out->c[1]=(value & 0x3F) | 0x80;
    out->len=2;
  }
  else if(value <=0xFFFF) {
    out->c[0]=(value >> 12) | 0xE0;
    out->c[1]=((value >> 6) & 0x3F) | 0x80;
    out->c[2]=(value & 0x3F) | 0x80;
    out->len=3;
  }
  else {
    out->c[0]=(value >> 18) | 0xF0;
    out->c[1]=((value >> 12) & 0x3F) | 0x80;
    out->c[2]=((value >> 6) & 0x3F) | 0x80;
    out->c[3]=(value & 0x3F) | 0x80;
    out->len=4;
  }
}

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
    if(isxdigit(c))
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
  unsigned int i;

  utf8_encode(x, &seq);
  for (i = 0; i < seq.len; ++i, d++)
    *d = seq.c[i];
  return d;
}

static int
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
        break; /*  break the while loop */
      }

      if (s == input_end) {
        /* input is not a well-formed json string */
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
      case	'u': {
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
          break; }
      default:
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
  char *start = t->buf + 1; /*  start after initial '"' */
  char *end = start;

  /*  reach the end of the string */
  while (*end != '\0' && *end != '"') {
    if ('\\' == *end++ && *end != '\0') { /*  check for escaped characters */
      ++end; /*  eat-up escaped character */
    }
  }
  if (*end != '"') return false; /*  make sure reach end of string */

  char * unscp_str = NULL;
  size_t unscp_len = 0;
  if (json_string_unescape(&unscp_str,  &unscp_len, start, end-start)) {
    if (unscp_str == start) {
      t->str = cee_str_mk_e(st, end-start+1, "%.*s", end-start, start);
    }
    else {
      /* / @todo? create a cee_str func that takes ownership of string */
      t->str = cee_str_mk_e(st, unscp_len+1, "%s", unscp_str);
      free(unscp_str);
    }
    t->buf = end + 1; /*  skip the closing '"' */
    return true;
  }
  return false; /*  ill formed string */
}

static bool parse_number(struct tokenizer *t) {
  char *start = t->buf;
  char *end = start;

  bool is_integer = true, is_exponent = false;
  int offset_sign = 0;

  /* 1st STEP: check for a minus sign and skip it */
  if ('-' == *end) {
    ++end;
    offset_sign = 1;
  }
  if (!isdigit(*end)) 
    return false;

  /* 2nd STEP: skips until a non digit char found */
  while (isdigit(*++end)) 
    continue;

  /* 3rd STEP: if non-digit char is not a comma then it must be an integer */
  if ('.' == *end) {
    if (!isdigit(*++end))
      return false;
    while (isdigit(*++end)) 
      continue;
    is_integer = false;
  }

  /* 4th STEP: check for exponent and skip its tokens */
  /*  TODO: check if its an integer or not and change 'is_integer' accordingly */
  if ('e' == *end || 'E' == *end) {
    ++end;
    if ('+' == *end || '-' == *end)
      ++end;
    if (!isdigit(*end)) 
      return false;
    while (isdigit(*++end))
      continue;
    is_exponent = true;
  } /*  can't be a integer that starts with zero followed n numbers (ex: 012, -023) */
  else if (is_integer && (end-1) != (start+offset_sign) && '0' == start[offset_sign]) {
    return false;
  }

  /* 5th STEP: convert string to number */
  int ret;
  if (is_exponent || !is_integer) {
    t->type = NUMBER_IS_DOUBLE;
    ret = sscanf(start, "%lf", &t->number.real);
  }
  else {
    t->type = NUMBER_IS_I64;
    ret = sscanf(start, "%"PRId64, &t->number.i64);
  }

  t->buf = end; /* skips entire length of number */

  return EOF != ret;
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
        if(check(t->buf, "/", &t->buf)) {
          while(t->buf < t->buf_end) {
            c = t->buf[0];
            if(c=='\0') return tock_eof;
            if(c=='\n') return tock_err; 
            t->buf++;
          }
        }
        return tock_err;
    default:
        return tock_err;
    }
  }
}
