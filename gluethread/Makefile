CC=gcc
CFLAGS=-g -c -I

test:test.o glthread.o
	gcc -g test.o glthread.o -o test

test.o:test.c
	gcc -g -c test.c -o test.o

glthread.o:glthread.c
	gcc -g -c glthread.c -o glthread.o

clear:
	rm *.o
	rm test