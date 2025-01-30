CC = gcc
CFLAGS = -Wall -ggdb

SRC = simulator.c queue.c
OBJ = simulator.o queue.o
TARGET = simulator

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lSDL2 -lSDL2_image

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJ)
