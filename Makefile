CC = clang
CCPP = clang++
CFLAGS = -c -g -Wall

SRCDIR = src
INCL = -I$(SRCDIR)

LLVMFLAGS = $(shell llvm-config --cxxflags)
LLVMLDFLAGS = $(shell llvm-config --ldflags)
LLVMLIBS = $(shell llvm-config --libs)

LIBS = $(LLVMLIBS)
FLAGS = $(INCL) $(LLVMFLAGS)

LDFLAGS = $(LLVMLDFLAGS) $(LIBS)

CFILES = \
    cfl_ast.o \
    cfl_ast.error.o \
    cfl_program.o \
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
    cfl_eval.o

all: libcfl.a cfl

libcfl.a: $(CFILES)
	ar cr libcfl.a $(CFILES)

CPPFILES = cfl_compiler.opp

cfl-core: cfl_main.opp $(CPPFILES) libcfl.a
	$(CCPP) -o cfl-core $< $(CPPFILES) -L. -lcfl $(LDFLAGS)

cfl: cfl-core

clean:
	rm -f *.o *.opp *.a

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $^ $(INCL)

%.opp: $(SRCDIR)/%.cpp
	$(CCPP) $(CFLAGS) -o $*.opp $^ $(FLAGS)
