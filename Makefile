CC = gcc
CFLAGS = -Wall -Wextra -std=c11
SRC = src/main.c src/grid_detection.c
OBJ = $(SRC:.c=.o)
TARGET = grid_split

INCLUDES = -Isrc
LIBS = -lm

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: all
	@echo "Cleaning old outputs..."
	@rm -f data/images/detected_grid.png
	@rm -f data/cells/*.png
	@echo "Running program..."
	./$(TARGET)
