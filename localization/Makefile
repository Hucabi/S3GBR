CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LIBS = -lgd -lm

# Source files
SOURCES = projection.c \
          grid_detection.c \
          letter_extraction.c \
          wordlist_detection.c \
          wordlist_extraction.c \
          main_with_wordlist.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Executable
TARGET = wordsearch_extractor

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c localization.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
	rm -f localization_debug.png
	rm -rf ../data/grid/cells/*.jpg
	rm -rf ../data/wordlist/cells/*.jpg

# Create necessary directories
setup:
	mkdir -p ../data/grid/cells
	mkdir -p ../data/wordlist/cells

# Run test
test: $(TARGET) setup
	./$(TARGET) test_image.png