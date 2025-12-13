// main_with_wordlist.c - Complete program with grid and wordlist extraction
#include "localization.h"
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <image_path>\n", argv[0]);
        printf("Example: %s wordsearch_binarized.png\n", argv[0]);
        return 1;
    }
    
    const char* image_path = argv[1];
    
    // Create output directories
    mkdir("../data", 0755);
    mkdir("../data/grid", 0755);
    mkdir("../data/grid/cells", 0755);
    mkdir("../data/wordlist", 0755);
    mkdir("../data/wordlist/cells", 0755);
    
    printf("=== OCR Wordsearch - Localization & Extraction ===\n");
    printf("Input image: %s\n\n", image_path);
    
    // Load image
    FILE* fp = fopen(image_path, "rb");
    if (!fp) {
        perror("Failed to open image");
        return 1;
    }
    
    gdImagePtr img = gdImageCreateFromPng(fp);
    fclose(fp);
    
    if (!img) {
        printf("Failed to load PNG image\n");
        return 1;
    }
    
    printf("Image loaded: %dx%d pixels\n\n", gdImageSX(img), gdImageSY(img));
    
    // ===== STEP 1: GRID DETECTION =====
    printf("[STEP 1] Grid Detection\n");
    printf("-----------------------------------\n");
    BoundingBox grid = find_grid_by_projection(img);
    
    if (grid.width == 0 || grid.height == 0) {
        printf("ERROR: Could not detect grid!\n");
        gdImageDestroy(img);
        return 1;
    }
    
    printf("✓ Grid found: x=%d, y=%d, width=%d, height=%d\n\n", 
           grid.x, grid.y, grid.width, grid.height);
    
    // ===== STEP 2: GRID LETTER EXTRACTION =====
    printf("[STEP 2] Grid Letter Extraction\n");
    printf("-----------------------------------\n");
    int rows, cols;
    LetterData** grid_cells = extract_grid_letters(img, grid, &rows, &cols);
    
    if (!grid_cells || rows == 0 || cols == 0) {
        printf("ERROR: Failed to extract grid cells!\n");
        gdImageDestroy(img);
        return 1;
    }
    
    printf("✓ Grid size: %d rows × %d columns\n", rows, cols);
    
    // Save grid cells as 32x32 JPG
    int saved_grid = 0;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (grid_cells[r][c].letter_img) {
                char filename[256];
                snprintf(filename, sizeof(filename), 
                        "../data/grid/cells/grid_%02d_%02d.jpg", r, c);
                save_cell_as_jpg(grid_cells[r][c].letter_img, filename, 32);
                saved_grid++;
            }
        }
    }
    printf("✓ Saved %d grid letters to ../data/grid/cells/\n\n", saved_grid);
    
    // ===== STEP 3: WORDLIST DETECTION =====
    printf("[STEP 3] Wordlist Detection\n");
    printf("-----------------------------------\n");
    BoundingBox wordlist = find_wordlist_region(img, grid);
    
    if (wordlist.width == 0 || wordlist.height == 0) {
        printf("⚠ No wordlist detected\n\n");
    } else {
        printf("✓ Wordlist found: x=%d, y=%d, width=%d, height=%d\n\n", 
               wordlist.x, wordlist.y, wordlist.width, wordlist.height);
        
        // ===== STEP 4: WORDLIST LETTER EXTRACTION =====
        printf("[STEP 4] Wordlist Letter Extraction\n");
        printf("-----------------------------------\n");
        
        int num_words;
        int* letters_per_word;
        LetterData** wordlist_letters = extract_wordlist_letters(img, wordlist, 
                                                                &num_words, 
                                                                &letters_per_word);
        
        if (wordlist_letters && num_words > 0) {
            // Save wordlist letters as 28x28 JPG
            save_wordlist_letters(wordlist_letters, num_words, letters_per_word, 
                                "../data/wordlist/cells");
            
            // Cleanup wordlist data
            for (int w = 0; w < num_words; w++) {
                for (int l = 0; l < letters_per_word[w]; l++) {
                    if (wordlist_letters[w][l].letter_img) {
                        gdImageDestroy(wordlist_letters[w][l].letter_img);
                    }
                }
                free(wordlist_letters[w]);
            }
            free(wordlist_letters);
            free(letters_per_word);
        }
        printf("\n");
    }
    
    // ===== STEP 5: DEBUG VISUALIZATION =====
    printf("[STEP 5] Debug Visualization\n");
    printf("-----------------------------------\n");
    
    gdImagePtr debug = gdImageCreate(gdImageSX(img), gdImageSY(img));
    
    // Allocate colors for debug image
    int white = gdImageColorAllocate(debug, 255, 255, 255);
    int black = gdImageColorAllocate(debug, 0, 0, 0);
    int red = gdImageColorAllocate(debug, 255, 0, 0);
    int green = gdImageColorAllocate(debug, 0, 255, 0);
    
    // Copy original image
    for (int y = 0; y < gdImageSY(img); y++) {
        for (int x = 0; x < gdImageSX(img); x++) {
            int pixel = gdImageGetPixel(img, x, y);
            // Map to our palette: 0=black, anything else=white
            gdImageSetPixel(debug, x, y, (pixel == 0) ? black : white);
        }
    }
    
    // Draw box around grid (thick red border)
    int thickness = 3;
    for (int t = 0; t < thickness; t++) {
        // Top and bottom
        for (int x = grid.x - t; x < grid.x + grid.width + t; x++) {
            if (x >= 0 && x < gdImageSX(debug)) {
                if (grid.y - t >= 0)
                    gdImageSetPixel(debug, x, grid.y - t, red);
                if (grid.y + grid.height + t < gdImageSY(debug))
                    gdImageSetPixel(debug, x, grid.y + grid.height + t, red);
            }
        }
        // Left and right
        for (int y = grid.y - t; y < grid.y + grid.height + t; y++) {
            if (y >= 0 && y < gdImageSY(debug)) {
                if (grid.x - t >= 0)
                    gdImageSetPixel(debug, grid.x - t, y, red);
                if (grid.x + grid.width + t < gdImageSX(debug))
                    gdImageSetPixel(debug, grid.x + grid.width + t, y, red);
            }
        }
    }
    
    // Draw box around wordlist if found (thick green border)
    if (wordlist.width > 0 && wordlist.height > 0) {
        for (int t = 0; t < thickness; t++) {
            for (int x = wordlist.x - t; x < wordlist.x + wordlist.width + t; x++) {
                if (x >= 0 && x < gdImageSX(debug)) {
                    if (wordlist.y - t >= 0)
                        gdImageSetPixel(debug, x, wordlist.y - t, green);
                    if (wordlist.y + wordlist.height + t < gdImageSY(debug))
                        gdImageSetPixel(debug, x, wordlist.y + wordlist.height + t, green);
                }
            }
            for (int y = wordlist.y - t; y < wordlist.y + wordlist.height + t; y++) {
                if (y >= 0 && y < gdImageSY(debug)) {
                    if (wordlist.x - t >= 0)
                        gdImageSetPixel(debug, wordlist.x - t, y, green);
                    if (wordlist.x + wordlist.width + t < gdImageSX(debug))
                        gdImageSetPixel(debug, wordlist.x + wordlist.width + t, y, green);
                }
            }
        }
    }
    
    // Save debug image
    FILE* debug_out = fopen("localization_debug.png", "wb");
    if (debug_out) {
        gdImagePng(debug, debug_out);
        fclose(debug_out);
        printf("✓ Debug visualization saved: localization_debug.png\n");
    }
    
    gdImageDestroy(debug);
    
    // ===== CLEANUP =====
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (grid_cells[r][c].letter_img) {
                gdImageDestroy(grid_cells[r][c].letter_img);
            }
        }
        free(grid_cells[r]);
    }
    free(grid_cells);
    
    gdImageDestroy(img);
    
    // ===== SUMMARY =====
    printf("\n");
    printf("=====================================\n");
    printf("           EXTRACTION COMPLETE       \n");
    printf("=====================================\n");
    printf("Grid cells (32×32):    ../data/grid/cells/\n");
    printf("Wordlist letters (28×28): ../data/wordlist/cells/\n");
    printf("Debug visualization:   localization_debug.png\n");
    printf("\n");
    
    return 0;
}