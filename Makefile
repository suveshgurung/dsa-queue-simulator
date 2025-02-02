CC = gcc
CFLAGS = -Wall -ggdb

SIM_SRC = simulator.c queue.c socket.c
SIM_OBJ = $(SIM_SRC:.c=.o)
SIM_TARGET = simulator

GEN_SRC = traffic-generator.c socket.c
GEN_OBJ = $(GEN_SRC:.c=.o)
GEN_TARGET = traffic-generator

.PHONY: all clean

all: $(SIM_TARGET) $(GEN_TARGET)

$(SIM_TARGET): $(SIM_OBJ)
	$(CC) $(CFLAGS) -o $(SIM_TARGET) $(SIM_OBJ) -lSDL2 -lSDL2_image

$(GEN_TARGET): $(GEN_OBJ)
	$(CC) $(CFLAGS) -o $(GEN_TARGET) $(GEN_OBJ) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(SIM_TARGET) $(SIM_OBJ) $(GEN_TARGET) $(GEN_OBJ)
