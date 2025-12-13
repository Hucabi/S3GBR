// letter_extraction.c - ROBUST VERSION with better border detection
#include "localization.h"
#include <string.h>
#include <stdio.h>

// Center a letter in its cell (remove excess whitespace)
static gdImagePtr center_letter(gdImagePtr cell_img) {
    int width = cell_img->sx;
    int height = cell_img->sy;
    
    // STEP 1: Clean bounding box detection with noise filtering
    int min_x = width, max_x = 0, min_y = height, max_y = 0;
    int pixel_count = 0;
    
    // Find tight bounding box of black pixels
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (gdImageGetPixel(cell_img, x, y) == 0) {
                pixel_count++;
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }
    
    // If no black pixels or too few (noise), return white image
    if (pixel_count < 5) {
        gdImagePtr centered = gdImageCreate(32, 32);
        int white = gdImageColorAllocate(centered, 255, 255, 255);
        gdImageFilledRectangle(centered, 0, 0, 31, 31, white);
        return centered;
    }
    
    // Add small padding (10% of dimension)
    int bbox_width = max_x - min_x + 1;
    int bbox_height = max_y - min_y + 1;
    int padding_x = bbox_width / 10;
    int padding_y = bbox_height / 10;
    
    min_x = (min_x > padding_x) ? min_x - padding_x : 0;
    max_x = (max_x + padding_x < width) ? max_x + padding_x : width - 1;
    min_y = (min_y > padding_y) ? min_y - padding_y : 0;
    max_y = (max_y + padding_y < height) ? max_y + padding_y : height - 1;
    
    bbox_width = max_x - min_x + 1;
    bbox_height = max_y - min_y + 1;
    
    // STEP 2: Calculate scaling while preserving aspect ratio
    float scale_x = 32.0f / bbox_width;
    float scale_y = 32.0f / bbox_height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y; // Use smaller scale
    
    int scaled_width = (int)(bbox_width * scale);
    int scaled_height = (int)(bbox_height * scale);
    
    // STEP 3: Create centered image with proper positioning
    gdImagePtr centered = gdImageCreate(32, 32);
    int white = gdImageColorAllocate(centered, 255, 255, 255);
    int black = gdImageColorAllocate(centered, 0, 0, 0);
    gdImageFilledRectangle(centered, 0, 0, 31, 31, white);
    
    // Calculate centering offsets
    int offset_x = (32 - scaled_width) / 2;
    int offset_y = (32 - scaled_height) / 2;
    
    // STEP 4: Scale and copy with simple nearest-neighbor
    for (int y = 0; y < scaled_height; y++) {
        for (int x = 0; x < scaled_width; x++) {
            int src_x = min_x + (int)(x / scale);
            int src_y = min_y + (int)(y / scale);
            
            if (src_x < width && src_y < height) {
                int pixel = gdImageGetPixel(cell_img, src_x, src_y);
                if (pixel == 0) { // Black pixel
                    gdImageSetPixel(centered, offset_x + x, offset_y + y, black);
                }
            }
        }
    }
    
    return centered;
}

// Helper: Find continuous border regions
static int* find_border_lines(int* projection, int length, int threshold, 
                              int min_gap, int* num_borders) {
    int* borders = (int*)malloc(length * sizeof(int));
    int count = 0;
    int in_border = 0;
    int border_start = 0;
    
    for (int i = 0; i < length; i++) {
        if (projection[i] > threshold) {
            if (!in_border) {
                border_start = i;
                in_border = 1;
            }
        } else {
            if (in_border) {
                // End of border region - store the middle
                int border_middle = (border_start + i - 1) / 2;
                
                // Only add if far enough from previous border
                if (count == 0 || border_middle - borders[count - 1] > min_gap) {
                    borders[count++] = border_middle;
                }
                in_border = 0;
            }
        }
    }
    
    // Handle border at end
    if (in_border) {
        int border_middle = (border_start + length - 1) / 2;
        if (count == 0 || border_middle - borders[count - 1] > min_gap) {
            borders[count++] = border_middle;
        }
    }
    
    *num_borders = count;
    return borders;
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

// Extract letters from GRID with IMPROVED border detection
LetterData** extract_grid_letters(gdImagePtr img, BoundingBox grid, 
                                 int* num_rows, int* num_cols) {
    printf("Extracting letters from grid (%dx%d)...\n", grid.width, grid.height);
    
    // 1. Compute projections within grid region
    int* h_proj = compute_horizontal_projection_within(img, grid);
    int* v_proj = compute_vertical_projection_within(img, grid);
    
    // 2. Adaptive thresholding for border detection
    // Borders should be VERY dark (high projection values)
    int h_border_threshold = grid.width * 0.7;  // 70% of width (increased from 50%)
    int v_border_threshold = grid.height * 0.7; // 70% of height
    
    // Estimate cell size to set minimum gap between borders
    int estimated_cell_height = grid.height / 15; // Assume 5-20 rows
    int estimated_cell_width = grid.width / 15;   // Assume 5-20 cols
    
    // Find continuous border lines
    int border_count_h, border_count_v;
    int* row_borders = find_border_lines(h_proj, grid.height, h_border_threshold, 
                                        estimated_cell_height / 2, &border_count_h);
    int* col_borders = find_border_lines(v_proj, grid.width, v_border_threshold, 
                                        estimated_cell_width / 2, &border_count_v);
    
    // Grid size = borders - 1
    *num_rows = border_count_h - 1;
    *num_cols = border_count_v - 1;
    
    printf("Detected %d row borders, %d col borders\n", border_count_h, border_count_v);
    printf("Grid size: %d rows x %d columns\n", *num_rows, *num_cols);
    
    // Sanity check: grid should be reasonable size
    if (*num_rows < 3 || *num_rows > 30 || *num_cols < 3 || *num_cols > 30) {
        printf("ERROR: Unrealistic grid size detected! Aborting.\n");
        free(h_proj);
        free(v_proj);
        free(row_borders);
        free(col_borders);
        *num_rows = 0;
        *num_cols = 0;
        return NULL;
    }
    
    // 4. Allocate 2D array for letters
    LetterData** letters = (LetterData**)malloc(*num_rows * sizeof(LetterData*));
    for (int i = 0; i < *num_rows; i++) {
        letters[i] = (LetterData*)malloc(*num_cols * sizeof(LetterData));
    }
    
    // 5. Extract each cell
    for (int row = 0; row < *num_rows; row++) {
        int cell_top = grid.y + row_borders[row] + 2;
        int cell_bottom = grid.y + row_borders[row + 1] - 2;
        
        for (int col = 0; col < *num_cols; col++) {
            int cell_left = grid.x + col_borders[col] + 2;
            int cell_right = grid.x + col_borders[col + 1] - 2;
            
            int cell_width = cell_right - cell_left + 1;
            int cell_height = cell_bottom - cell_top + 1;
            
            // Safety check
            if (cell_width <= 0 || cell_height <= 0) {
                printf("WARNING: Invalid cell at [%d,%d]\n", row, col);
                continue;
            }
            
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