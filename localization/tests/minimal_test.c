#include "../localization.h"
#include <sys/stat.h>

int main() {
    printf("=== Minimal Localization Test ===\n");
    
    const char* image_path = "../data/images/level_1_binarized.png";
    
    // Create output directories
    mkdir("../data/grid/cells", 0755);
    
    // Load image
    FILE* fp = fopen(image_path, "rb");
    if (!fp) {
        printf("Cannot open image: %s\n", image_path);
        return 1;
    }
    
    gdImagePtr img = gdImageCreateFromPng(fp);
    fclose(fp);
    
    if (!img) {
        printf("Failed to load image\n");
        return 1;
    }
    
    printf("Image loaded: %dx%d pixels\n", gdImageSX(img), gdImageSY(img));
    
    // Test projection functions
    printf("\n1. Testing projection functions...\n");
    int* h_proj = compute_horizontal_projection(img);
    int* v_proj = compute_vertical_projection(img);
    
    // Print some projection values
    printf("Horizontal projection (first 30 rows):\n");
    for (int i = 0; i < 30 && i < gdImageSY(img); i++) {
        if (h_proj[i] > 0) {
            printf("  Row %3d: %4d black pixels\n", i, h_proj[i]);
        }
    }
    
    // Detect grid
    printf("\n2. Detecting grid...\n");
    BoundingBox grid = find_grid_by_projection(img);
    printf("   Grid found: x=%d, y=%d, w=%d, h=%d\n", 
           grid.x, grid.y, grid.width, grid.height);
    
    if (grid.width == 0 || grid.height == 0) {
        printf("ERROR: No grid detected!\n");
        free(h_proj);
        free(v_proj);
        gdImageDestroy(img);
        return 1;
    }
    
    // Test word list detection
    printf("\n3. Detecting word list...\n");
    BoundingBox wordlist = find_wordlist_region(img, grid);
    if (wordlist.width > 0 && wordlist.height > 0) {
        printf("   Word list found: x=%d, y=%d, w=%d, h=%d\n", 
               wordlist.x, wordlist.y, wordlist.width, wordlist.height);
    } else {
        printf("   No word list detected\n");
    }
    
    // Extract a few letters as test
    printf("\n4. Testing letter extraction...\n");
    int rows, cols;
    LetterData** letters = extract_grid_letters(img, grid, &rows, &cols);
    
    if (letters && rows > 0 && cols > 0) {
        printf("   Grid size: %d rows x %d columns\n", rows, cols);
        
        // Save first few letters
        int save_count = 0;
        for (int r = 0; r < rows && r < 2; r++) {
            for (int c = 0; c < cols && c < 2; c++) {
                if (letters[r][c].letter_img) {
                    char filename[256];
                    snprintf(filename, sizeof(filename), 
                            "../data/grid/cells/mini_%d_%d.jpg", r, c);
                    
                    save_cell_as_jpg(letters[r][c].letter_img, filename, 32);
                    printf("   Saved: %s\n", filename);
                    save_count++;
                }
            }
        }
        printf("   Total saved: %d test cells\n", save_count);
        
        // Clean up letters
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (letters[r][c].letter_img) {
                    gdImageDestroy(letters[r][c].letter_img);
                }
            }
            free(letters[r]);
        }
        free(letters);
    }
    
    // Cleanup
    free(h_proj);
    free(v_proj);
    gdImageDestroy(img);
    
    printf("\n=== Test Complete ===\n");
    return 0;
}