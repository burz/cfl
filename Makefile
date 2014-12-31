CC = gcc
CFLAGS = -c -g

SRCDIR = src

INCL = -I$(SRCDIR)

FILES = \
    cfl.o

all: cfl

cfl: cfl.o $(FILES)
	$(CC) -o cfl $^

clean:
	rm -f *.o

%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $^ $(INCL)
