
CC = gcc

# Important we need to turn optimization on for the cpu driver
CFLAGS = -O3 -march=native -funroll-loops -Wall -I.. #-D DEBUG6522

all: via1 via2 vic load save

via1:
	$(CC) $(CFLAGS) -c -o via1.o via1.c
	$(CC) $(CFLAGS) via1.o -fPIC -shared -l pthread -o via1.so 

via2:
	$(CC) $(CFLAGS) -c -o via2.o via2.c
	$(CC) $(CFLAGS) via2.o -fPIC -shared -lpthread -o via2.so

vic:
	$(CC) $(CFLAGS) -c -o vic.o vic.c #-DREALTIME -DDEBUG -DDEBUGIO
	$(CC) $(CFLAGS) vic.o -fPIC -shared -lpthread -lasound -o vic.so  

load:
	$(CC) $(CFLAGS) -c -o load.o load.c 
	$(CC) $(CFLAGS) -fPIC -shared -o load.so load.o 

save:
	$(CC) $(CFLAGS) -c -o save.o save.c 
	$(CC) $(CFLAGS) -fPIC -shared -o save.so save.o 

clean:
	rm -f *.o *.so
