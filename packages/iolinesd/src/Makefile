CC = gcc
CFLAGS = -Os -Wall -Wmissing-declarations -g3

PROGRAMS=iolinesd

all: $(PROGRAMS)

iolinesd: iolinesd.o conf.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o $(PROGRAMS)
