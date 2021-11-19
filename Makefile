CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic -O3
LDFLAGS =  -pthread

SOURCES = spx.c handle_incoming.c handle_outgoing.c helper.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = spx

$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean

clean:
	@rm -f $(TARGET) $(OBJECTS) core