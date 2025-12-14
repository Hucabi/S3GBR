#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <gd.h>
#include <stdio.h>
#include <stdlib.h>

// --- Structs ---
typedef struct {
    int x, y;
    int width, height;
} BoundingBox;

// --- Projection Utils (projection.c) ---
int* compute_vertical_projection(gdImagePtr img);
int* compute_horizontal_projection(gdImagePtr img);
// Helper for finding grid in specific area
int* compute_vertical_projection_within(gdImagePtr img, BoundingBox area);
int* compute_horizontal_projection_within(gdImagePtr img, BoundingBox area);

// --- Grid Detection (grid_detection.c) ---
BoundingBox find_grid_by_projection(gdImagePtr img);
// Legacy fallback signature
BoundingBox find_grid_by_text_clustering(gdImagePtr img);
void flood_fill(gdImagePtr img, int x, int y, BoundingBox* bbox, int** visited);

// --- Grid Extraction (letter_extraction.c) ---
// UPDATED: Now returns void because it saves directly to disk
void extract_grid_letters(gdImagePtr img, BoundingBox grid_box);

// --- Wordlist Detection (wordlist_detection.c) ---
BoundingBox find_wordlist_region(gdImagePtr img, BoundingBox grid);

// --- Wordlist Extraction (wordlist_extraction.c) ---
// UPDATED: Now returns void because it saves directly to disk
void extract_wordlist_letters(gdImagePtr img, BoundingBox box);

// --- Debugging ---
void save_debug_image(gdImagePtr img, char* filename, BoundingBox grid, BoundingBox wordlist);

#endif