#ifndef WORD_LIST_DETECTION_H
#define WORD_LIST_DETECTION_H

#include "grid_detection.h"

typedef struct {
    	int count;
    	char **words;
    	Rectangle *word_rects;
    	Rectangle list_region;
} WordList;

WordList find_word_list(BinaryImage *img, GridCells grid);

void visualize_word_list(BinaryImage *original,
		WordList word_list,
		const char *output_path);

unsigned char* binary_pixel_at(BinaryImage *img, int x, int y);

void save_word_list_cells(BinaryImage *img,
		WordList word_list,
		const char *base_path);

void save_word_letters(BinaryImage *img,
		WordList word_list,
		const char *base_path);

void word_list_free(WordList *word_list);

#endif
