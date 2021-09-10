CC ?= gcc
CEE_SRC = musl-hsearch.c musl-insque.c musl-lsearch.c musl-tsearch.c \
          cee-common.c boxed.c str.c dict.c map.c set.c stack.c      \
          tuple.c triple.c quadruple.c list.c tagged.c singleton.c   \
          closure.c block.c n_tuple.c env.c state.c 

CEE_UTILS_DIR  := cee-utils

CFLAGS= -fno-exceptions -g -I./ -I./cee-utils

.PHONY: release clean distclean

all: cee_utils tester

cee_utils: $(CEE_UTILS_DIR)

tester: cee.c
	$(CC) $(CFLAGS) $@.c -o $@.out $<

echo:
	@ echo "$(CEE_SRC)"

cee.c: $(CEE_SRC)
	@ cat $(CEE_SRC) | awk \
	'BEGIN {\
		print "#ifndef CEE_ONE_H";\
		print "#define CEE_ONE_H";\
		print "#define CEE_AMALGAMATION";\
		print "#include \"cee.h\"";\
		print "#include <stdlib.h>";\
		print "#include <string.h>";\
		print "#include <stdio.h>";\
		print "#include <errno.h>";\
		print "#include \"cee-internal.h\"";\
		print "#include \"musl-search.h\"";\
	}\
	{print $$0}\
	END {\
		print "#endif // CEE_ONE_H";\
	}' > $@

$(CEE_UTILS_DIR):
	if [[ ! -d $@ ]]; then                \
	  ./scripts/get-cee-utils.sh || exit; \
	fi

clean:
	rm -f cee.c tmp.c one.* a.out
