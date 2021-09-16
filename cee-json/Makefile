TOP = ..
CC ?= gcc

TEST_DIR = test

JSON_SRC = value.c parser.c snprint.c tokenizer.c

CFLAGS= -fno-exceptions -g -I./ -I$(TOP)/ -I$(TOP)/cee-utils

.PHONY: release clean test

# generic compilation
%.out: %.c cee-json-one.o cee-one.o
	$(CC) $(CFLAGS) -std=c11 -g -o $@ $^

all: cee.o cee-json-one.o

cee-one.o: $(TOP)/cee-one.c
	$(MAKE) -C $(TOP)
	$(CC) $(CFLAGS) -c -g $<

cee-json-one.c: $(JSON_SRC)
	@ cat $(JSON_SRC) | awk \
	'BEGIN {\
		print "#ifndef CEE_JSON_ONE_H";\
		print "#define CEE_JSON_ONE_H";\
		print "#define CEE_JSON_AMALGAMATION";\
		print "#define _GNU_SOURCE";\
		print "#include <stdlib.h>";\
		print "#include <string.h>";\
		print "#include <stdio.h>";\
		print "#include <errno.h>";\
		print "#include <sys/types.h>";\
		print "#include <sys/stat.h>";\
		print "#include <unistd.h>";\
		print "#include \"cee.h\"";\
		print "#include \"utf8.h\"";\
		print "#include \"tokenizer.h\"";\
		print "#include \"cee-json.h\"";\
	}\
	{print $$0}\
	END {\
		print "#endif // CEE_JSON_ONE_H";\
	}' > $@

cee-json-one.o: cee-json-one.c $(TOP)/cee.h
	$(CC) $(CFLAGS) -c cee-json-one.c

release: $(JSON_SRC)
	@ mkdir -p release
	@ $(MAKE) cee-json-one.c
	@ mv cee-json.c release
	@ cp cee-json.h release

test: all
	$(MAKE) -C $(TEST_DIR) -f test.mk

clean:
	rm -f a.c cee-one.o cee-json-one.c cee-json-one.o
	$(MAKE) -C $(TEST_DIR) -f test.mk clean
