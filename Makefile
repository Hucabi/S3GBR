CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LIBS = -lm

# Chemins
SRC_DIR = src
BUILD_DIR = build
DATA_DIR = data

# Fichiers sources
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/grid_detection.c $(SRC_DIR)/word_list_detection.c
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = wordsearch_solver

# Créer le dossier build si nécessaire
$(shell mkdir -p $(BUILD_DIR))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/grid_detection.h $(SRC_DIR)/word_list_detection.h
$(BUILD_DIR)/grid_detection.o: $(SRC_DIR)/grid_detection.c $(SRC_DIR)/grid_detection.h
$(BUILD_DIR)/word_list_detection.o: $(SRC_DIR)/word_list_detection.c $(SRC_DIR)/word_list_detection.h $(SRC_DIR)/grid_detection.h

clean:
	rm -rf $(BUILD_DIR) $(TARGET) grid_detection.png grid_cell_*.png

# Créer les dossiers de données
init:
	mkdir -p $(DATA_DIR)/cells $(DATA_DIR)/images

.PHONY: all clean init
