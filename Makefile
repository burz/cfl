CC = clang
CCPP = clang++
CFLAGS = -c -g -Wall

SRCDIR = src
INCL = -I$(SRCDIR)

LLVMFLAGS = $(shell llvm-config --cflags)
LLVMLDFLAGS = $(shell llvm-config --ldflags)
LLVMLIBS = $(shell llvm-config --libs)

LIBS = -L. $(LLVMLIBS)

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

cfl: $(SRCDIR)/cfl_main.cpp libcfl.a
	$(CCPP) $(CFLAGS) $(SRCDIR)/cfl_compiler.cpp $(FLAGS)
	$(CCPP) -o cfl $< cfl_compiler.o -lcfl $(LDFLAGS)

clean:
	rm -f *.o *.a

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $^ $(FLAGS)
