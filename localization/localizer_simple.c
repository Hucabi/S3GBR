#include "localization.h"
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <image_path> [output_dir]\n", argv[0]);
        printf("Example: %s ../data/images/level_1_binarized.png\n", argv[0]);
        return 1;
    }
    
    const char* image_path = argv[1];
    const char* output_dir = (argc > 2) ? argv[2] : "extracted";
    
    // Create directories
    mkdir(output_dir, 0755);
    mkdir("../data/grid/cells", 0755);
    mkdir("../data/wordlist/cells", 0755);
    
    printf("=== Simple Localization & Extraction ===\n");
    printf("Image: %s\n", image_path);
    
    // Load image
    FILE* fp = fopen(image_path, "rb");
    if (!fp) {
        perror("Failed to open image");
        return 1;
    }
    
    gdImagePtr img = gdImageCreateFromPng(fp);
    fclose(fp);
    
    if (!img) {
        printf("Failed to load image\n");
        return 1;
    }
    
    printf("Image size: %dx%d\n", gdImageSX(img), gdImageSY(img));
    
    // 1. Detect grid
    printf("\n1. Grid detection...\n");
    BoundingBox grid = find_grid_by_projection(img);
    printf("   Grid: x=%d, y=%d, w=%d, h=%d\n", 
           grid.x, grid.y, grid.width, grid.height);
    
    if (grid.width == 0 || grid.height == 0) {
        printf("ERROR: No grid found!\n");
        gdImageDestroy(img);
        return 1;
    }
    
    // 2. Extract grid cells
    printf("\n2. Grid cell extraction...\n");
    int rows, cols;
    LetterData** grid_cells = extract_grid_letters(img, grid, &rows, &cols);
    
    if (!grid_cells || rows == 0 || cols == 0) {
        printf("ERROR: Failed to extract grid cells\n");
        gdImageDestroy(img);
        return 1;
    }
    
    printf("   Grid: %d rows x %d columns\n", rows, cols);
    
    // Save grid cells
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
    printf("   Saved %d grid cells\n", saved_grid);
    
    // 3. Detect word list (search all around grid)
    printf("\n3. Word list detection...\n");
    BoundingBox wordlist = find_wordlist_region(img, grid);
    
    if (wordlist.width > 0 && wordlist.height > 0) {
        printf("   Word list: x=%d, y=%d, w=%d, h=%d\n", 
               wordlist.x, wordlist.y, wordlist.width, wordlist.height);
        
        // For now, just save the entire word list region as debug
        gdImagePtr wordlist_img = gdImageCreate(wordlist.width, wordlist.height);
        for (int y = 0; y < wordlist.height; y++) {
            for (int x = 0; x < wordlist.width; x++) {
                int color = gdImageGetPixel(img, wordlist.x + x, wordlist.y + y);
                gdImageSetPixel(wordlist_img, x, y, color);
            }
        }
        
        FILE* wl_out = fopen("../data/wordlist/wordlist_debug.png", "wb");
        if (wl_out) {
            gdImagePng(wordlist_img, wl_out);
            fclose(wl_out);
            printf("   Saved word list debug image\n");
        }
        gdImageDestroy(wordlist_img);
    } else {
        printf("   No word list detected\n");
    }
    
    // 4. Create debug visualization
    printf("\n4. Creating debug visualization...\n");
    gdImagePtr debug = gdImageCreate(gdImageSX(img), gdImageSY(img));
    
    // Copy original
    for (int y = 0; y < gdImageSY(img); y++) {
        for (int x = 0; x < gdImageSX(img); x++) {
            gdImageSetPixel(debug, x, y, gdImageGetPixel(img, x, y));
        }
    }
    
    // Draw red box around grid
    for (int x = grid.x; x < grid.x + grid.width && x < gdImageSX(debug); x++) {
        if (x == grid.x || x == grid.x + grid.width - 1) {
            for (int y = grid.y; y < grid.y + grid.height && y < gdImageSY(debug); y++) {
                gdImageSetPixel(debug, x, y, 0); // Black border
            }
        }
        gdImageSetPixel(debug, x, grid.y, 0);
        gdImageSetPixel(debug, x, grid.y + grid.height - 1, 0);
    }
    
    // Draw green box around word list if found
    if (wordlist.width > 0) {
        for (int x = wordlist.x; x < wordlist.x + wordlist.width && x < gdImageSX(debug); x++) {
            if (x == wordlist.x || x == wordlist.x + wordlist.width - 1) {
                for (int y = wordlist.y; y < wordlist.y + wordlist.height && y < gdImageSY(debug); y++) {
                    gdImageSetPixel(debug, x, y, 0);
                }
            }
            gdImageSetPixel(debug, x, wordlist.y, 0);
            gdImageSetPixel(debug, x, wordlist.y + wordlist.height - 1, 0);
        }
    }
    
    FILE* debug_out = fopen("localization_debug.png", "wb");
    if (debug_out) {
        gdImagePng(debug, debug_out);
        fclose(debug_out);
        printf("   Saved: localization_debug.png\n");
    }
    
    // Cleanup
    gdImageDestroy(debug);
    
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
    
    printf("\n=== Done ===\n");
    printf("Grid cells: ../data/grid/cells/grid_XX_XX.jpg\n");
    printf("Debug: localization_debug.png\n");
    
    return 0;
}