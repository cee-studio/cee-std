SHELL := /bin/bash
CC ?= gcc

OBJDIR        = obj
TESTDIR       = test
CEE_UTILS_DIR = cee-utils

MUSL_SRC = musl-hsearch.c musl-insque.c musl-lsearch.c musl-tsearch.c
CEE_SRC  = cee-common.c boxed.c str.c dict.c map.c set.c stack.c tuple.c triple.c \
           quadruple.c list.c tagged.c singleton.c closure.c block.c n_tuple.c env.c state.c 
SRC      = $(MUSL_SRC) $(CEE_SRC)
STD_HDR  = stdio.h string.h stdlib.h stdarg.h assert.h errno.h
OBJS    = $(SRC:%.c=$(OBJDIR)/%.o)

CFLAGS := -std=c99 -g -I./ -I./$(CEE_UTILS_DIR)

ifeq ($(LCOV),1)
	CFLAGS  += --coverage
	LDFLAGS += --coverage
endif

define cee_amalgamation
	@echo "#define CEE_AMALGAMATION" > tmp.c
	@for ii in $(SRC); do echo '#include "'$$ii'"' >> tmp.c; done
	@echo "#ifndef CEE_ONE" > $(1)
	@echo "#define CEE_ONE" >> $(1)
	@echo "#define _GNU_SOURCE" >> $(1)
	@for ii in $(STD_HDR); do echo '#include <'$$ii'>' >> $(1); done
	@cat cee.h >> $(1)
	@cat musl-search.h >> $(1)
	@echo " " >> $(1)
	$(CC) -E -CC $(2) -nostdinc tmp.c >> $(1)
	@echo "#endif" >> $(1)
	@rm -f tmp.c
endef

COMMIT_SHA = $(shell git log -n 1 --pretty=format:\"%H\")

.PHONY: release clean distclean

# generic compilation
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: cee_utils $(OBJS) cee-one.c

cee_utils: $(CEE_UTILS_DIR)
$(OBJS): | $(OBJDIR)

$(CEE_UTILS_DIR):
	if [[ ! -d $@ ]]; then                \
	  ./scripts/get-cee-utils.sh || exit; \
	fi
$(OBJDIR):
	@mkdir -p $(OBJDIR)

cee-one.c: $(SRC)
	$(call cee_amalgamation, cee-one.c)

release: all
	$(call cee_amalgamation, cee.c,-P)
	@mkdir -p release
	@mv cee.c  release
	@cp cee.h  release
	$(MAKE) -C cee-json release

test: all
	$(MAKE) -C $(TESTDIR) -f test.mk
	$(TESTDIR)/test-func.out
	$(TESTDIR)/test-memory.out
	$(MAKE) -C cee-json test_parse test_print

lcov:
	$(MAKE) LCOV=1 release
	lcov --directory . --zerocounters -q
	$(MAKE) LCOV=1 -k test
	lcov --directory . -c -o /tmp/cee-std_test.info
	lcov --remove /tmp/cee-std_test.info "*/test/*" "*/cee-utils/*" -o /tmp/cee-std_coverage.info
	genhtml /tmp/cee-std_coverage.info --legend --title  "commit $(COMMIT_SHA)" --output-directory lcov-out

echo:
	@ echo $(COMMIT_SHA)
	@ echo "$(SRC)"

clean:
	rm -rf cee-one.c cee.c tmp.c a.out
	rm -rf $(OBJDIR)
	$(MAKE) -C $(TESTDIR) -f test.mk clean
	$(MAKE) -C cee-json clean
	rm -rf lcov-out /tmp/cee-std_*.info
	lcov --zerocounters --directory .
