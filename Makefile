CC = gcc
CFLAGS = -Wall -ggdb

all: traffic-simulator

traffic-simulator: traffic-simulator.o
	$(CC) $(CFLAGS) -o traffic-simulator traffic-simulator.o

traffic-simulator.o: traffic-simulator.c
	$(CC) $(CFLAGS) -c traffic-simulator.c -o traffic-simulator.o

clean:
	rm -rf traffic-simulator traffic-simulator.o
