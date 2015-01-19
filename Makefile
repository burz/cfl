CC = gcc
CFLAGS = -c -g -Wall

SRCDIR = src

INCL = -I$(SRCDIR)

FILES = \
    cfl_ast.o \
    cfl_ast.error.o \
    cfl_program.o \
    cfl_parser.o \
    cfl_parser.error.o \
    cfl_parser.token.o \
    cfl_parser.basic.o \
    cfl_parser.operator.o \
    cfl_parser.derived.o \
    cfl_eval.o \
    cfl_type.o \
    cfl_type.error.o \
    cfl_type.equation.o \
    cfl_type.generate.o \
    cfl_type.program.o

ifneq ($(OS), Windows_NT)
    UNAME = $(shell uname -s)
    ifeq ($(UNAME), Linux)
        FILES += cfl_eval.posix.o
    else
        ifeq ($(UNAME), Darwin)
            FILES += cfl_eval.posix.o
        else
            FILES += cfl_eval.generic.c
        endif
    endif
else
    FILES += cfl_eval.generic.c
endif

all: cfl

cfl: cfl_main.o $(FILES)
	$(CC) -o cfl $^

clean:
	rm -f *.o

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $^ $(INCL)
