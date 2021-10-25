TOP = ..
CC ?= gcc

SRC   = $(wildcard *.c)
EXES  = $(SRC:%.c=%.out)

CFLAGS := -std=c99 -fno-exceptions -g -I$(TOP)/ -I$(TOP)/cee-utils -static

ifeq ($(LCOV),1)
	CFLAGS  += --coverage
	LDFLAGS += --coverage -static
endif

# generic compilation
%.out: %.c
	$(CC) $(CFLAGS) -g -o $@ $^

all: $(EXES)

clean:
	rm -f *.out *.gcno *.gcda

.PHONY : all clean
