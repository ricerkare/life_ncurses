CC=gcc

all: life

life: life.c
	$(CC) life.c -o life -lncurses -lm

clean:
	rm -f *.o
