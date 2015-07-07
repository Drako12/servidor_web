CC = gcc
CFLAGS = -g -Wall -Wextra
IDIR = ../include
TARGET = server
SOURCES = server.c main.c
OBJ = $(SOURCES:.c=.o)

all: $SOURCES $(TARGET)

$TARGET: ($OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

.PHONY: clean

clean:
	 rm -f *.o
