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

GENERIC_FILES = \
    cfl_malloc.generic.o \
    cfl_eval.generic.o

POSIX_FILES = \
    cfl_malloc.posix.o \
    cfl_eval.posix.o

ifneq ($(OS), Windows_NT)
    UNAME = $(shell uname -s)
    ifeq ($(UNAME), Linux)
        FILES += $(POSIX_FILES)
    else
        ifeq ($(UNAME), Darwin)
            FILES += $(POSIX_FILES)
        else
            FILES += $(GENERIC_FILES)
        endif
    endif
else
    FILES += $(GENERIC_FILES)
endif

all: cfl

cfl: cfl_main.o $(FILES)
	$(CC) -o cfl $^

clean:
	rm -f *.o

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $^ $(INCL)
