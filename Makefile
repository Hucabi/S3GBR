# Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
SRCDIR = src
BUILDDIR = build
TARGET = wordsearch_solver

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) -lm

all: $(TARGET)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

.PHONY: all clean
