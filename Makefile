CC ?= gcc
CEE_SRC = musl-hsearch.c musl-insque.c musl-lsearch.c musl-tsearch.c \
          cee-common.c boxed.c str.c dict.c map.c set.c stack.c      \
          tuple.c triple.c quadruple.c list.c tagged.c singleton.c   \
          closure.c block.c n_tuple.c env.c state.c 

CFLAGS= -fno-exceptions -g -I./

.PHONY: release clean distclean

all: tester

tester:
	$(CC) $(CFLAGS) tester.c $(CEE_SRC)

clean:
	rm -f cee.c tmp.c cee-one.* a.out
