
CC = gcc

# Important we need to turn optimization on for the cpu driver
CFLAGS = -O3 -march=native -funroll-loops -Wall -I.. 

all:
	$(CC) $(CFLAGS) -c -o fb_basic.o fb_basic.c 
	$(CC) $(CFLAGS) -fPIC -shared -o fb_basic.so fb_basic.o 

clean:
	rm -f *.o *.so
