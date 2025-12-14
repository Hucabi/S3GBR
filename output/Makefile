CC = gcc
CFLAGS = -Wall -Wextra -lpng
TARGET = output.c

all: $(TARGET)
	$(CC) $(CFLAGS) -o output $(TARGET)

clean:
	rm -f output output.png

.PHONY: all clean
