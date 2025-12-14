#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <gd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int x, y;
    int width, height;
} BoundingBox;

int* compute_vertical_projection(gdImagePtr img);
int* compute_horizontal_projection(gdImagePtr img);
int* compute_vertical_projection_within(gdImagePtr img, BoundingBox area);
int* compute_horizontal_projection_within(gdImagePtr img, BoundingBox area);
BoundingBox find_grid_by_projection(gdImagePtr img);
BoundingBox find_grid_by_text_clustering(gdImagePtr img);
void flood_fill(gdImagePtr img, int x, int y, BoundingBox* bbox,
		int** visited);
void extract_grid_letters(gdImagePtr img, BoundingBox grid_box);
BoundingBox find_wordlist_region(gdImagePtr img, BoundingBox grid);
void extract_wordlist_letters(gdImagePtr img, BoundingBox box);
void save_debug_image(gdImagePtr img, char* filename, BoundingBox grid,
		BoundingBox wordlist);

#endif
