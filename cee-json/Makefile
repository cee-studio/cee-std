TOP = ..
CC ?= gcc

TEST_DIR = test

JSON_SRC = value.c parser.c snprint.c tokenizer.c
JSON_HDR = cee-json.h tokenizer.h utf8.h

HEADERS = stdlib.h string.h errno.h sys/types.h sys/stat.h unistd.h stdio.h

define json_amalgamation
	@echo "#ifndef CEE_JSON_ONE" > $(1)
	@echo "#define CEE_JSON_ONE" >> $(1)
	@echo "#define _GNU_SOURCE" >> $(1)
	@for ii in $(HEADERS); do echo '#include <'$$ii'>' >> $(1); done
	@echo "#include \"$(TOP)/cee.h\"" >> $(1)
	@echo " " >> $(1)
	@for ii in $(JSON_HDR); do cat $$ii >> $(1); echo " " >> $(1); done
	@echo "#define CEE_JSON_AMALGAMATION" > tmp.c
	@for ii in $(JSON_SRC); do echo '#include "'$$ii'"' >> tmp.c; done  
	$(CC) -E $(2) -nostdinc tmp.c >> $(1)
	@echo "#endif" >> $(1)
endef

CFLAGS= -fno-exceptions -g -I./ -I$(TOP)/ -I$(TOP)/cee-utils

.PHONY: release clean test

# generic compilation
%.out: %.c cee-json-one.o cee.o
	$(CC) $(CFLAGS) -std=c11 -g -o $@ $^

all: cee.o cee-json-one.o

cee.o: $(TOP)/cee.c
	$(MAKE) -C $(TOP)
	$(CC) $(CFLAGS) -c -g $<

cee-json-one.c: $(JSON_SRC) $(TOP)/cee.h
	$(call json_amalgamation, cee-json-one.c)
cee-json-one.o: cee-json-one.c $(TOP)/cee.h
	$(CC) $(CFLAGS) -c cee-json-one.c

release: $(JSON_SRC)
	@mkdir -p release
	$(call json_amalgamation, cee-json.c, -P)
	@mv cee-json.c release
	@cp cee-json.h release

test: all
	$(MAKE) -C $(TEST_DIR) -f test.mk

clean:
	rm -f a.c cee.o cee-json-one.c cee-json-one.o
	$(MAKE) -C $(TEST_DIR) -f test.mk clean
