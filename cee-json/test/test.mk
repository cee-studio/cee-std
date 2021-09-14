TOP = ../..
CC ?= gcc

SRC   = $(wildcard *.c)
EXES  = $(filter %.out, $(SRC:.c=.out))

SUITE_EXE           = test-cee-json.out
SUITE_PARSING_DIR   = test_parsing
SUITE_TRANSFORM_DIR = test_transform

CFLAGS= -fno-exceptions -g -I../ -I$(TOP)/ -I$(TOP)/cee-utils

# generic compilation
%.out: %.c ../cee-json-one.o ../cee.o
	$(CC) $(CFLAGS) -std=c11 -g -o $@ $^

all: $(EXES) $(SUITE_PARSING_DIR) $(SUITE_TRANSFORM_DIR)
	- ./$(SUITE_EXE) -e -s json_parsing -- $(SUITE_PARSING_DIR)/*
	- ./$(SUITE_EXE) -e -s json_transform -- $(SUITE_TRANSFORM_DIR)/*

clean:
	rm -f *.out

.PHONY : all clean
