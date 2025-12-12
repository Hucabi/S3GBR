// letter_extraction.c - CORRECTED VERSION
#include "localization.h"
#include <string.h>
#include <stdio.h>

static void find_letter_bounding_box(gdImagePtr cell_img, 
                                     int* min_x, int* max_x, 
                                     int* min_y, int* max_y) {
    int width = gdImageSX(cell_img);
    int height = gdImageSY(cell_img);
    
    // Initialize with extreme values
    *min_x = width;
    *max_x = 0;
    *min_y = height;
    *max_y = 0;
    
    int found_black = 0;
    
    // Scan the entire cell for black pixels (index 0)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int color = gdImageGetPixel(cell_img, x, y);
            if (color == 0) {  // BLACK pixel
                found_black = 1;
                if (x < *min_x) *min_x = x;
                if (x > *max_x) *max_x = x;
                if (y < *min_y) *min_y = y;
                if (y > *max_y) *max_y = y;
            }
        }
    }
    
    // If no black pixels found, set to center of cell
    if (!found_black) {
        *min_x = width / 2 - 1;
        *max_x = width / 2 + 1;
        *min_y = height / 2 - 1;
        *max_y = height / 2 + 1;
    }
}

// Center a letter in its cell (remove excess whitespace)
gdImagePtr center_letter(gdImagePtr cell_img) {
    int width = gdImageSX(cell_img);
    int height = gdImageSY(cell_img);
    
    // Find bounding box using the helper function
    int min_x, max_x, min_y, max_y;
    find_letter_bounding_box(cell_img, &min_x, &max_x, &min_y, &max_y);
    
    // Add padding
    int padding = 1;
    min_x = (min_x - padding > 0) ? min_x - padding : 0;
    min_y = (min_y - padding > 0) ? min_y - padding : 0;
    max_x = (max_x + padding < width) ? max_x + padding : width - 1;
    max_y = (max_y + padding < height) ? max_y + padding : height - 1;
    
    int letter_width = max_x - min_x + 1;
    int letter_height = max_y - min_y + 1;
    
    // Create centered image (32x32)
    int target_size = 32;
    gdImagePtr centered = gdImageCreate(target_size, target_size);
    
    // Fill with white
    if (gdImageTrueColor(centered)) {
        int white = gdImageColorAllocate(centered, 255, 255, 255);
        gdImageFill(centered, 0, 0, white);
    } else {
        for (int y = 0; y < target_size; y++) {
            for (int x = 0; x < target_size; x++) {
                gdImageSetPixel(centered, x, y, 255);
            }
        }
    }
    
    // Calculate scaling
    float scale_x = (float)target_size / letter_width;
    float scale_y = (float)target_size / letter_height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y; // Keep aspect ratio
    
    int scaled_width = (int)(letter_width * scale);
    int scaled_height = (int)(letter_height * scale);
    
    int offset_x = (target_size - scaled_width) / 2;
    int offset_y = (target_size - scaled_height) / 2;
    
    // Copy and scale the letter
    for (int y = 0; y < scaled_height; y++) {
        for (int x = 0; x < scaled_width; x++) {
            int src_x = min_x + (int)(x / scale);
            int src_y = min_y + (int)(y / scale);
            
            if (src_x >= 0 && src_x < width && src_y >= 0 && src_y < height) {
                int color = gdImageGetPixel(cell_img, src_x, src_y);
                if (color == 0) { // Only copy black pixels
                    gdImageSetPixel(centered, offset_x + x, offset_y + y, 0);
                }
            }
        }
    }
    
    return centered;
}

// New function for resizing and binarizing to target size
gdImagePtr resize_and_binarize(gdImagePtr src, int target_size) {
    gdImagePtr dst = gdImageCreate(target_size, target_size);
    
    // Fill with white
    int white = gdImageColorAllocate(dst, 255, 255, 255);
    gdImageFill(dst, 0, 0, white);
    
    // Resize
    double scale_x = (double)target_size / gdImageSX(src);
    double scale_y = (double)target_size / gdImageSY(src);
    
    for (int y = 0; y < target_size; y++) {
        for (int x = 0; x < target_size; x++) {
            int src_x = (int)(x / scale_x);
            int src_y = (int)(y / scale_y);
            
            if (src_x >= 0 && src_x < gdImageSX(src) && 
                src_y >= 0 && src_y < gdImageSY(src)) {
                int color = gdImageGetPixel(src, src_x, src_y);
                
                // Convert to black if pixel is dark enough
                if (gdImageTrueColor(src)) {
                    int r = gdImageRed(src, color);
                    int g = gdImageGreen(src, color);
                    int b = gdImageBlue(src, color);
                    
                    if (r < 128 && g < 128 && b < 128) {
                        gdImageSetPixel(dst, x, y, 0); // Black
                    }
                } else {
                    // Palette image
                    if (color == 0) { // Assuming 0 is black
                        gdImageSetPixel(dst, x, y, 0);
                    }
                }
            }
        }
    }
    
    // Convert to palette with only black and white
    gdImageTrueColorToPalette(dst, 1, 2);
    
    return dst;
}

// Extract letters from grid with automatic row/col detection
LetterData** extract_grid_letters(gdImagePtr img, BoundingBox grid, 
                                 int* num_rows, int* num_cols) {
    printf("Extracting letters from grid (%dx%d)...\n", grid.width, grid.height);
    
    // 1. Compute projections within grid region
    int* h_proj = compute_horizontal_projection_within(img, grid);
    int* v_proj = compute_vertical_projection_within(img, grid);
    
    // 2. Find borders (high projection values = cell separators)
    int h_border_threshold = grid.width * 0.5; // 50% of width
    int v_border_threshold = grid.height * 0.5; // 50% of height
    
    // Count borders to determine grid size
    int border_count_h = 0;
    for (int y = 0; y < grid.height; y++) {
        if (h_proj[y] > h_border_threshold) {
            border_count_h++;
        }
    }
    
    int border_count_v = 0;
    for (int x = 0; x < grid.width; x++) {
        if (v_proj[x] > v_border_threshold) {
            border_count_v++;
        }
    }
    
    // Grid size = borders - 1
    *num_rows = border_count_h - 1;
    *num_cols = border_count_v - 1;
    
    printf("Detected %d rows x %d columns grid\n", *num_rows, *num_cols);
    
    // 3. Find exact border positions
    int* row_borders = (int*)malloc(border_count_h * sizeof(int));
    int* col_borders = (int*)malloc(border_count_v * sizeof(int));
    
    int idx = 0;
    for (int y = 0; y < grid.height; y++) {
        if (h_proj[y] > h_border_threshold) {
            row_borders[idx++] = y;
        }
    }
    
    idx = 0;
    for (int x = 0; x < grid.width; x++) {
        if (v_proj[x] > v_border_threshold) {
            col_borders[idx++] = x;
        }
    }
    
    // 4. Allocate 2D array for letters
    LetterData** letters = (LetterData**)malloc(*num_rows * sizeof(LetterData*));
    for (int i = 0; i < *num_rows; i++) {
        letters[i] = (LetterData*)malloc(*num_cols * sizeof(LetterData));
    }
    
    // 5. Extract each cell
    for (int row = 0; row < *num_rows; row++) {
        int cell_top = grid.y + row_borders[row] + 1;
        int cell_bottom = grid.y + row_borders[row + 1] - 1;
        
        for (int col = 0; col < *num_cols; col++) {
            int cell_left = grid.x + col_borders[col] + 1;
            int cell_right = grid.x + col_borders[col + 1] - 1;
            
            int cell_width = cell_right - cell_left + 1;
            int cell_height = cell_bottom - cell_top + 1;
            
            // Extract raw cell image
            gdImagePtr cell_img = gdImageCreate(cell_width, cell_height);
            for (int y = 0; y < cell_height; y++) {
                for (int x = 0; x < cell_width; x++) {
                    int color = gdImageGetPixel(img, cell_left + x, cell_top + y);
                    gdImageSetPixel(cell_img, x, y, color);
                }
            }
            
            // Center and normalize the letter
            gdImagePtr centered_img = center_letter(cell_img);
            gdImageDestroy(cell_img);
            
            // Store in structure
            letters[row][col].bbox.x = cell_left;
            letters[row][col].bbox.y = cell_top;
            letters[row][col].bbox.width = cell_width;
            letters[row][col].bbox.height = cell_height;
            letters[row][col].letter_img = centered_img;
            letters[row][col].row = row;
            letters[row][col].col = col;
            letters[row][col].predicted_char = '?';
        }
    }
    
    // Cleanup
    free(h_proj);
    free(v_proj);
    free(row_borders);
    free(col_borders);
    
    printf("Extracted %d letters total\n", (*num_rows) * (*num_cols));
    
    return letters;
}

// Extract letters from word list - STUB FOR NOW
LetterData** extract_wordlist_letters(gdImagePtr img, BoundingBox wordlist, 
                                     int* num_words, int** letters_per_word) {
    // Mark parameters as unused to avoid warnings
    (void)img;
    (void)wordlist;
    
    // This is simpler: words are horizontal, separated by whitespace
    printf("Extracting letters from word list...\n");
    
    // For now, return empty structure
    *num_words = 0;
    *letters_per_word = NULL;
    
    printf("Word list extraction not implemented yet\n");
    
    return NULL;
}

// Helper function to save a cell as JPEG
void save_cell_as_jpg(gdImagePtr cell, const char* filename, int target_size) {
    FILE* out = fopen(filename, "wb");
    if (!out) {
        perror("Failed to open file for writing");
        return;
    }
    
    // Resize to target size if needed
    gdImagePtr resized = cell;
    if (gdImageSX(cell) != target_size || gdImageSY(cell) != target_size) {
        resized = resize_and_binarize(cell, target_size);
    } else {
        // Create a copy to avoid modifying original
        resized = gdImageCreate(target_size, target_size);
        gdImageCopy(resized, cell, 0, 0, 0, 0, target_size, target_size);
    }
    
    // Ensure it's binarized (black & white palette)
    gdImageTrueColorToPalette(resized, 1, 2);
    
    // Save as JPEG
    gdImageJpeg(resized, out, 95); // 95% quality
    
    fclose(out);
    
    // Clean up if we created a resized copy
    if (resized != cell) {
        gdImageDestroy(resized);
    }
}