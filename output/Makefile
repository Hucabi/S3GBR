CC = gcc
CFLAGS = -Wall -Wextra
TARGET = output.c

all: $(TARGET)
	$(CC) $(CFLAGS) -o output $(TARGET) -lpng

clean:
	rm -f output output.png

.PHONY: all clean
