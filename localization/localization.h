// localization.h - CORRECTED VERSION
#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <gd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Constants for output format
#define CELL_SIZE 32  // 32x32 pixels
#define GRID_OUTPUT_DIR "../data/grid/cells"
#define WORDLIST_OUTPUT_DIR "../data/wordlist/cells"

// Core data structures
typedef struct {
    int x;
    int y;
    int width;
    int height;
} BoundingBox;

typedef struct {
    BoundingBox bbox;
    gdImagePtr letter_img;
    char predicted_char;
    int row;
    int col;
} LetterData;

typedef struct {
    BoundingBox grid_bbox;
    BoundingBox wordlist_bbox;
    int grid_rows;
    int grid_cols;
    LetterData** grid_letters;
    LetterData** wordlist_letters;
    int num_words;
} ExtractionResult;

// ========== PROJECTION FUNCTIONS ==========
int* compute_horizontal_projection(gdImagePtr img);
int* compute_vertical_projection(gdImagePtr img);
int* compute_horizontal_projection_within(gdImagePtr img, BoundingBox region);
int* compute_vertical_projection_within(gdImagePtr img, BoundingBox region);
int* find_peaks(int* projection, int length, int min_height, int min_distance, int* num_peaks);
int* find_valleys(int* projection, int length, int max_height, int min_distance, int* num_valleys);

// ========== GRID DETECTION FUNCTIONS ==========
BoundingBox find_grid_by_projection(gdImagePtr img);
BoundingBox find_grid_by_text_clustering(gdImagePtr img);
void flood_fill(gdImagePtr img, int x, int y, BoundingBox* bbox, int** visited);

// ========== LETTER EXTRACTION FUNCTIONS ==========
LetterData** extract_grid_letters(gdImagePtr img, BoundingBox grid, 
                                 int* num_rows, int* num_cols);
LetterData** extract_wordlist_letters(gdImagePtr img, BoundingBox wordlist, 
                                     int* num_words, int** letters_per_word);
gdImagePtr center_letter(gdImagePtr cell_img);

// ========== WORD LIST FUNCTIONS ==========
BoundingBox find_wordlist_region(gdImagePtr img, BoundingBox grid);

// ========== IMAGE PROCESSING FUNCTIONS ==========
gdImagePtr resize_and_binarize(gdImagePtr src, int target_size);
void save_cell_as_jpg(gdImagePtr cell, const char* filename, int target_size);

// ========== MAIN FUNCTIONS ==========
ExtractionResult* localize_and_extract(gdImagePtr binarized_img);
void free_extraction_result(ExtractionResult* result);
void save_debug_images(ExtractionResult* result, const char* base_name);

#endif