#include "../localization.h"
#include <sys/stat.h>

int main() {
    const char* image_path = "../data/images/level_1_binarized.png";
    
    // Create output directories
    mkdir("../data/grid/cells", 0755);
    mkdir("../data/wordlist/cells", 0755);
    
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
    
    printf("Image loaded: %dx%d\n", gdImageSX(img), gdImageSY(img));
    
    // Detect grid
    BoundingBox grid = find_grid_by_projection(img);
    printf("Grid: x=%d, y=%d, w=%d, h=%d\n", 
           grid.x, grid.y, grid.width, grid.height);
    
    // Extract letters
    int rows, cols;
    LetterData** letters = extract_grid_letters(img, grid, &rows, &cols);
    
    printf("Grid: %d rows x %d cols\n", rows, cols);
    
    // Save first few letters as test
    if (letters && rows > 0 && cols > 0) {
        for (int r = 0; r < rows && r < 3; r++) {
            for (int c = 0; c < cols && c < 3; c++) {
                if (letters[r][c].letter_img) {
                    char filename[256];
                    snprintf(filename, sizeof(filename), 
                            "../data/grid/cells/test_%d_%d.jpg", r, c);
                    
                    // Save as 32x32 JPEG
                    save_cell_as_jpg(letters[r][c].letter_img, filename, 32);
                    printf("Saved: %s\n", filename);
                }
            }
        }
    }
    
    // Cleanup
    if (letters) {
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
    
    gdImageDestroy(img);
    
    return 0;
}