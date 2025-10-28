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

#endif
