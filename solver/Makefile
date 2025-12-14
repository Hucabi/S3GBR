CC = gcc
CFLAGS = -Wall -Wextra
TARGET = solver.c

all: $(TARGET)
	$(CC) $(CFLAGS) -o solver $(TARGET) 

1solv: solver1.c
	$(CC) $(CFLAGS) -o solver solver1.c

clean:
	rm -f solver coords.txt grid.txt words.txt

.PHONY: all clean solv1
