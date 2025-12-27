
CC = gcc

# Important we need to turn optimization on for the cpu driver
CFLAGS = -O3 -march=native -funroll-loops -Wall -I.. 

all:
	$(CC) $(CFLAGS) -c -o fake6502.o fake6502.c #-DSTEPDEBUG
	$(CC) $(CFLAGS) -o monitor monitor.c fake6502.o -D FAST

clean:
	rm -f *.o monitor