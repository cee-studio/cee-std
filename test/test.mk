TOP = ..
CC ?= gcc

OBJS = $(wildcard ../obj/*.o)

SRC   = $(wildcard *.c)
EXES  = $(SRC:%.c=%.out)

CFLAGS := -std=c99 -fno-exceptions -g -I$(TOP)/ -I$(TOP)/cee-utils

ifeq ($(LCOV),1)
	CFLAGS  += --coverage
	LDFLAGS += --coverage
endif

# generic compilation
%.out: %.c $(OBJS)
	$(CC) $(CFLAGS) -g -o $@ $^

all: $(EXES)

clean:
	rm -f *.out *.gcno *.gcda

.PHONY : all clean
