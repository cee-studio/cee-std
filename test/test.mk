TOP = ..
CC ?= gcc

OBJS = $(wildcard ../obj/*)

SRC   = $(wildcard *.c)
EXES  = $(SRC:%.c=%.out)

CFLAGS= -fno-exceptions -g -I$(TOP)/ -I$(TOP)/cee-utils

# generic compilation
%.out: %.c $(OBJS)
	$(CC) $(CFLAGS) -g -o $@ $^

all: $(EXES)

clean:
	rm -f *.out

.PHONY : all clean
