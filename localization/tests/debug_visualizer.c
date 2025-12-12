#include "localization.h"

void save_debug_images(ExtractionResult* result, const char* base_name) {
    char filename[256];
    
    // 1. Save grid with bounding box
    // (You would create a copy of the original image and draw rectangles)
    
    // 2. Save each extracted grid letter
    for (int row = 0; row < result->grid_rows; row++) {
        for (int col = 0; col < result->grid_cols; col++) {
            snprintf(filename, sizeof(filename), "%s_grid_%02d_%02d.png", 
                     base_name, row, col);
            
            FILE* fp = fopen(filename, "wb");
            if (fp) {
                gdImagePng(result->grid_letters[row][col].letter_img, fp);
                fclose(fp);
            }
        }
    }
    
    // 3. Save word list letters
    for (int word = 0; word < result->num_words; word++) {
        // Get letters for this word (implementation depends on your structure)
        // Save each letter...
    }
}