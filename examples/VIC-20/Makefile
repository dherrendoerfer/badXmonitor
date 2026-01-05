
CC = gcc

# Important we need to turn optimization on for the cpu driver
CFLAGS = -O3 -march=native -funroll-loops -Wall -I.. #-D DEBUG6522

all: via1 via2 vic

via1:
	$(CC) $(CFLAGS) -c -o via1.o via1.c 
	$(CC) $(CFLAGS) -fPIC -shared -o via1.so via1.o 

via2:
	$(CC) $(CFLAGS) -c -o via2.o via2.c 
	$(CC) $(CFLAGS) -fPIC -shared -o via2.so via2.o 

vic:
	$(CC) $(CFLAGS) -c -o vic.o vic.c 
	$(CC) $(CFLAGS) -fPIC -shared -o vic.so vic.o 

clean:
	rm -f *.o *.so
