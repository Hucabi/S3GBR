# localization/Makefile - UPDATED
CC = gcc
CFLAGS = -Wall -Wextra -g -I.
LDFLAGS = -lgd -lpng -ljpeg -lm

# Source files
SRCS = projection.c grid_detection.c letter_extraction.c wordlist_detection.c
OBJS = $(SRCS:.c=.o)

# Test programs
TESTS = tests/test_working_extraction tests/simple_extract tests/test_grid

# Main library
liblocalization.a: $(OBJS)
	ar rcs $@ $(OBJS)

# Pattern rule for object files
%.o: %.c localization.h
	$(CC) $(CFLAGS) -c $< -o $@

# Test programs
tests/test_working_extraction: tests/test_working_extraction.c
	$(CC) $(CFLAGS) -o $@ tests/test_working_extraction.c $(LDFLAGS)

tests/simple_extract: tests/simple_extract.c liblocalization.a
	$(CC) $(CFLAGS) -o $@ tests/simple_extract.c -L. -llocalization $(LDFLAGS)

tests/test_grid: tests/test_grid.c liblocalization.a
	$(CC) $(CFLAGS) -o $@ tests/test_grid.c -L. -llocalization $(LDFLAGS)

# Main program
localizer_simple: localizer_simple.c liblocalization.a
	$(CC) $(CFLAGS) -o $@ localizer_simple.c -L. -llocalization $(LDFLAGS)

# Build all
all: liblocalization.a localizer_simple $(TESTS)

# Clean everything (including data directories)
clean:
	rm -f *.o liblocalization.a
	rm -f $(TESTS) localizer_simple
	rm -f debug_grid.png debug_localization.png localization_debug.png
	rm -f ../data/grid/cells/*.jpg ../data/grid/cells/*.png
	rm -f ../data/wordlist/cells/*.jpg ../data/wordlist/cells/*.png
	rm -f ../data/wordlist/*.png

.PHONY: all clean