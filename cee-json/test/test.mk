TOP = ../..
CC ?= gcc

OBJS = $(wildcard ../obj/*)

SRC   = $(wildcard *.c)
EXES  = $(SRC:%.c=%.out)

SUITE_EXE           = test-cee-json.out
SUITE_PARSING_DIR   = test_parsing
SUITE_TRANSFORM_DIR = test_transform

CFLAGS= -fno-exceptions -g -I../ -I$(TOP)/ -I$(TOP)/cee-utils

# generic compilation
%.out: %.c $(OBJS) $(TOP)/cee-one.c
	$(CC) $(CFLAGS) -g -o $@ $^

all: $(EXES) $(SUITE_PARSING_DIR) $(SUITE_TRANSFORM_DIR)
	- ./$(SUITE_EXE) -e -s json_parsing -- $(SUITE_PARSING_DIR)/*
	- ./$(SUITE_EXE) -e -s json_transform -- $(SUITE_TRANSFORM_DIR)/*

clean:
	rm -f *.out

.PHONY : all clean
