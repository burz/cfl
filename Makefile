CC = clang
CFLAGS = -c -g -Wall

SRCDIR = src
INCL = -I$(SRCDIR)

LLVMFLAGS = $(shell llvm-config --cflags)
LLVMLDFLAGS = $(shell llvm-config --ldflags)
LLVMLIBS = $(shell llvm-config --libs)

LIBS = $(LLVMLIBS)

FLAGS = $(INCL) $(LLVMFLAGS)
LDFLAGS = $(LLVMLDFLAGS) $(LIBS)

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
    cfl_type.o \
    cfl_type.error.o \
    cfl_type.equation.o \
    cfl_type.generate.o \
    cfl_type.program.o \
    cfl_eval.o \
    cfl_compiler.o

all: cfl

cfl: cfl_main.o $(FILES)
	$(CC) -o cfl $^ $(LDFLAGS)

clean:
	rm -f *.o

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $^ $(FLAGS)
