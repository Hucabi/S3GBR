CC = gcc
CFLAGS = pkg-config -Wall -Wextra gtk+-3.0
TARGET = ui.c

all: $(TARGET)
	$(CC) $(CFLAGS) -o ui $(TARGET)

clean:
	rm -f ui

.PHONY: all clean

CC = gcc

CFLAGS = -Wall -Wextra $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0)

TARGET = ui
SRC = ui.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
