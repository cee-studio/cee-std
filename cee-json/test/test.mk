TOP = ../..
CC ?= gcc

OBJS = $(wildcard ../obj/*)

SRC   = $(wildcard *.c)
EXES  = $(SRC:%.c=%.out)

TEST_PARSING_DIR   = test_parsing
TEST_TRANSFORM_DIR = test_transform
PARSER_SUITE_EXE   = test-parser.out
PRINT_SUITE_EXE    = test-print.out

CFLAGS= -fno-exceptions -g -I../ -I$(TOP)/ -I$(TOP)/cee-utils

# generic compilation
%.out: %.c $(OBJS) $(TOP)/cee-one.c
	$(CC) $(CFLAGS) -g -o $@ $^ $(TOP)/cee-utils/cJSON.c

test_parse: $(EXES) $(TEST_PARSING_DIR) $(TEST_TRANSFORM_DIR)
	- ./$(PARSER_SUITE_EXE) -e -s json_parsing -- $(TEST_PARSING_DIR)/*
	- ./$(PARSER_SUITE_EXE) -e -s json_transform -- $(TEST_TRANSFORM_DIR)/*

test_print: $(EXES) $(TEST_PARSING_DIR) $(TEST_TRANSFORM_DIR)
	- ./$(PRINT_SUITE_EXE) -s json_parsing -t cmp -- $(TEST_PARSING_DIR)/*
	- ./$(PRINT_SUITE_EXE) -s json_transform -t cmp -- $(TEST_TRANSFORM_DIR)/*

clean:
	rm -f *.out

.PHONY : all clean
