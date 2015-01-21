CC = clang
CCPP = clang++
CFLAGS = -c -g -Wall

SRCDIR = src
INCL = -Iinclude

LLVM = $(shell llvm-config 2> /dev/null; echo $$?)
ifeq ($(LLVM), 127)
    MAIN = llvm-not-found cfl-core-c
else
    MAIN = cfl-core
endif

LLVMFLAGS = $(shell llvm-config --cxxflags)
LLVMLDFLAGS = $(shell llvm-config --ldflags)
LLVMLIBS = $(shell llvm-config --libs core)

LIBS = $(LLVMLIBS)
FLAGS = $(INCL) $(LLVMFLAGS)

LDFLAGS = $(LLVMLDFLAGS) $(LIBS)

CFILES = \
    cfl_ast.o \
    cfl_ast.error.o \
    cfl_program.o \
    cfl_typed_program.o \
    cfl_parser.o \
    cfl_parser.error.o \
    cfl_parser.token.o \
    cfl_parser.basic.o \
    cfl_parser.operator.o \
    cfl_parser.derived.o \
    cfl_type.o \
    cfl_type.error.o \
    cfl_type.equation.o \
    cfl_type.generate.o \
    cfl_type.program.o \
    cfl_type.typed_program.o \
    cfl_eval.o

cfl: $(MAIN)

all: libcfl.a cfl-core-c cfl-core

libcfl.a: $(CFILES)
	ar cr libcfl.a $(CFILES)

llvm-not-found:
	@echo "LLVM not found. Building eval-only C version..."

cfl-core-c: cfl_main.o libcfl.a
	$(CC) -o cfl-core-c $< -L. -lcfl

CPPFILES = cfl_compiler.opp

cfl-core: cfl_main.opp $(CPPFILES) libcfl.a
	$(CCPP) -o cfl-core $< $(CPPFILES) -L. -lcfl $(LDFLAGS)

clean:
	rm -f *.o *.opp *.a

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $^ $(INCL)

%.opp: $(SRCDIR)/%.cpp
	$(CCPP) $(CFLAGS) -o $*.opp $^ $(FLAGS)
