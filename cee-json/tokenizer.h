#ifndef CEE_JSON_TOKENIZER_H
#define CEE_JSON_TOKENIZER_H
enum token {
  tock_eof = 255,
  tock_err,
  tock_str,
  tock_number,
  tock_true,
  tock_false,
  tock_null
};

struct tokenizer {
  int line;
  char * buf;
  char * buf_end;
  struct cee_str * str;
  double real;
};

extern enum token cee_json_next_token(struct cee_state *, struct tokenizer * t);
#endif // CEE_JSON_TOKENIZER_H
