CC = gcc
CFLAGS = -Wall -ggdb

SRC = simulator.c
OBJ = simulator.o
TARGET = simulator

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lSDL2

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

clean:
	rm -rf $(TARGET) $(OBJ)
