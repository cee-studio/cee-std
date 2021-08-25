TOP = ..
CC ?= gcc

JSON_SRC = value.c parser.c snprint.c tokenizer.c
JSON_HDR = json.h tokenizer.h utf8.h

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

CFLAGS= -fno-exceptions -g -I../

.PHONY: release clean

all: tester

json-one.c: $(JSON_SRC) $(TOP)/cee.h
	$(call json_amalgamation, json-one.c)

json-one.o: json-one.c $(TOP)/cee.h
	$(CC) $(CFLAGS) -c json-one.c

cee.o: $(TOP)/cee.c $(TOP)/cee.h
	$(CC) $(CFLAGS) -c -g $<

release: $(JSON_SRC)
	@mkdir -p release
	$(call json_amalgamation, json.c, -P)
	@mv json.c release
	@cp json.h release

tester: json-one.o cee.o
	$(CC) $(CFLAGS) -std=c11 -g $@.c -o $@.out $^

clean:
	rm -f a.c cee.o json-one.c json-one.o
