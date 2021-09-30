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
  double real; // to be deleted
  enum number_type {
     NUMBER_IS_DOUBLE,
     NUMBER_IS_I64,
     NUMBER_IS_U64
  } type;
  union {
    double real;
    int64_t i64;
    uint64_t u64;
  } number;
};

extern enum token cee_json_next_token(struct cee_state *, struct tokenizer * t);
#endif // CEE_JSON_TOKENIZER_H
