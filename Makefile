CC = gcc
CFLAGS = -c -g -Wall

SRCDIR = src

INCL = -I$(SRCDIR)

FILES = \
	cfl_ast.o \
    cfl_parser.o \
    cfl_eval.o

all: cfl

cfl: cfl_main.o $(FILES)
	$(CC) -o cfl $^

clean:
	rm -f *.o

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $^ $(INCL)
