SHELL := /bin/bash
CC ?= gcc

MUSL_SRC = musl-hsearch.c musl-insque.c musl-lsearch.c musl-tsearch.c
CEE_SRC  = $(MUSL_SRC) cee-common.c boxed.c str.c dict.c map.c set.c stack.c tuple.c triple.c \
           quadruple.c list.c tagged.c singleton.c closure.c block.c n_tuple.c env.c state.c 
HEADERS  = stdio.h string.h stdlib.h stdarg.h assert.h errno.h

CEE_UTILS_DIR = cee-utils

CFLAGS = -fno-exceptions -g -I./ -I./$(CEE_UTILS_DIR)

define cee_amalgamation
	@echo "#define CEE_AMALGAMATION" > tmp.c
	@for ii in $(CEE_SRC); do echo '#include "'$$ii'"' >> tmp.c; done
	@echo "#ifndef CEE_ONE" > $(1)
	@echo "#define CEE_ONE" >> $(1)
	@echo "#define _GNU_SOURCE" >> $(1)
	@for ii in $(HEADERS); do echo '#include <'$$ii'>' >> $(1); done
	@cat cee.h >> $(1)
	@cat musl-search.h >> $(1)
	@echo " " >> $(1)
	$(CC) -E -CC $(2) -nostdinc tmp.c >> $(1)
	@echo "#endif" >> $(1)
	@rm -f tmp.c
endef

.PHONY: release clean distclean

all: cee_utils tester

cee_utils: $(CEE_UTILS_DIR)

echo:
	@ echo "$(CEE_SRC)"

cee-one.c: $(CEE_SRC)
	$(call cee_amalgamation, cee-one.c)

cee-one.o: cee-one.c
	$(CC) -c -g $<

release:
	$(call cee_amalgamation, cee.c,-P)
	@mkdir -p release
	@mv cee.c  release
	@cp cee.h  release

tester: cee-one.o
	$(CC) -static -g tester.c cee-one.o

$(CEE_UTILS_DIR):
	if [[ ! -d $@ ]]; then                \
	  ./scripts/get-cee-utils.sh || exit; \
	fi

clean:
	rm -f cee-one.* cee.c tmp.c one.* a.out
