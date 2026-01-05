
CC = gcc

# Important we need to turn optimization on for the cpu driver
CFLAGS = -O3 -march=native -funroll-loops -Wall -I.. 

all:
	$(CC) $(CFLAGS) -c -o bad6502.o bad6502.c #-DSTEPDEBUG
	$(CC) $(CFLAGS) -o monitor monitor.c bad6502.o -ldl -D FAST

test-bin: all
	cp monitor ../test-bin

clean:
	rm -f *.o monitor
