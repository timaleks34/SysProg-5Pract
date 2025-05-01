CC := /usr/bin/gcc

all: clean | task1 task2

task1:
	$(CC) -o 51.o src/5-1-signal.c
task2:
	$(CC) -o 52.o src/5-2-unnamed.c

clean: 
	rm -rf *.o
