TOP = ..
CC ?= gcc

OBJDIR  = obj
TESTDIR = test

SRC     = value.c parser.c snprint.c tokenizer.c
HDR     = cee-json.h tokenizer.h 
STD_HDR = stdlib.h string.h errno.h sys/types.h sys/stat.h unistd.h stdio.h ctype.h
OBJS    = $(SRC:%.c=$(OBJDIR)/%.o)

CFLAGS= -std=c89 -fno-exceptions -g -I./ -I$(TOP)/ -I$(TOP)/cee-utils

.PHONY: release clean test

define cee_json_amalgamation
	@echo "#ifndef CEE_JSON_ONE" > $(1)
	@echo "#define CEE_JSON_ONE" >> $(1)
	@echo "#define _GNU_SOURCE" >> $(1)
	@for ii in $(STD_HDR); do echo '#include <'$$ii'>' >> $(1); done
	@echo "#include \"cee.h\"" >> $(1)
	@echo " " >> $(1)
	@for ii in $(HDR); do cat $$ii >> $(1); echo " " >> $(1); done
	@echo "#define CEE_JSON_AMALGAMATION" > tmp.c
	@for ii in $(SRC); do echo '#include "'$$ii'"' >> tmp.c; done  
	$(CC) -E $(2) -nostdinc tmp.c >> $(1)
	@echo "#endif" >> $(1)
	@rm -f tmp.c
endef

# generic compilation
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(OBJS) cee-json-one.c

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

cee-json-one.c: $(TOP)/cee-one.c $(SRC)
	$(call cee_json_amalgamation, $@)

release: all
	$(call cee_json_amalgamation, cee-json.c,-P)
	@mkdir -p release
	@mv cee-json.c release
	@cp cee-json.h release

test: all
	$(MAKE) -C $(TOP) release
	$(MAKE) -C $(TESTDIR) -f test.mk

clean:
	rm -rf a.c cee-json-one.c
	rm -rf release $(OBJDIR)
	$(MAKE) -C $(TESTDIR) -f test.mk clean
