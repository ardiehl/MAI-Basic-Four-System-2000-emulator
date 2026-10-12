TARGET = dumprel
LIBS =
CC = gcc
CFLAGS = -g0 -Os -Wall -g -I../../src/Musashi -I../../src
EXE = $(TARGET)

.PHONY: default all clean

default: $(TARGET)
all: default
Debug: default

SOURCES = $(wildcard *.c) ../../src/Musashi/m68kdasm.c ../../src/argparse.c
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
HEADERS = $(wildcard *.h) ../../src/Musashi/m68k.h ../../src/argparse.h

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $(EXE)

clean:
	-rm -f *.o
	-rm -f $(TARGET)
