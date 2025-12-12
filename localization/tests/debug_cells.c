// tests/debug_cells.c - DEBUG PROGRAM
#include "../localization.h"
#include <sys/stat.h>

// Helper to print pixel values
void print_cell_pixels(gdImagePtr img, int x, int y, int width, int height) {
    printf("Cell at (%d,%d) size %dx%d:\n", x, y, width, height);
    
    int black_count = 0;
    int white_count = 0;
    
    for (int row = 0; row < height && row < 10; row++) {
        printf("  Row %d: ", row);
        for (int col = 0; col < width && col < 10; col++) {
            int pixel = gdImageGetPixel(img, x + col, y + row);
            
            // Check if true color or palette
            if (gdImageTrueColor(img)) {
                int r = gdImageRed(img, pixel);
                int g = gdImageGreen(img, pixel);
                int b = gdImageBlue(img, pixel);
                if (r < 128 && g < 128 && b < 128) {
                    printf("B ");
                    black_count++;
                } else {
                    printf(". ");
                    white_count++;
                }
            } else {
                // Palette image
                if (pixel == 0) {
                    printf("B ");
                    black_count++;
                } else {
                    printf(". ");
                    white_count++;
                }
            }
        }
        printf("...\n");
    }
    printf("  Black pixels: %d, White pixels: %d\n", black_count, white_count);
}

// Test specific cell extraction
void test_single_cell(gdImagePtr img, int cell_x, int cell_y, int cell_width, int cell_height) {
    printf("\n=== Testing Cell Extraction ===\n");
    printf("Extracting cell at: x=%d, y=%d, w=%d, h=%d\n", 
           cell_x, cell_y, cell_width, cell_height);
    
    // 1. Print pixels from original image
    print_cell_pixels(img, cell_x, cell_y, cell_width, cell_height);
    
    // 2. Create the cell image
    gdImagePtr cell_img = gdImageCreate(cell_width, cell_height);
    
    // Copy pixels
    for (int y = 0; y < cell_height; y++) {
        for (int x = 0; x < cell_width; x++) {
            int color = gdImageGetPixel(img, cell_x + x, cell_y + y);
            gdImageSetPixel(cell_img, x, y, color);
        }
    }
    
    // 3. Save raw cell (before processing)
    FILE* raw = fopen("../data/grid/cells/debug_raw_cell.png", "wb");
    if (raw) {
        gdImagePng(cell_img, raw);
        fclose(raw);
        printf("Saved raw cell: debug_raw_cell.png\n");
    }
    
    // 4. Check what's in the cell image
    printf("\nIn cell image:\n");
    int black_in_cell = 0;
    for (int y = 0; y < 5; y++) {
        printf("  Row %d: ", y);
        for (int x = 0; x < 5; x++) {
            int pixel = gdImageGetPixel(cell_img, x, y);
            if (pixel == 0) {
                printf("B ");
                black_in_cell++;
            } else {
                printf(". ");
            }
        }
        printf("...\n");
    }
    
    // 5. Process and save
    gdImagePtr processed = resize_and_binarize(cell_img, 32);
    FILE* proc = fopen("../data/grid/cells/debug_processed.jpg", "wb");
    if (proc) {
        gdImageJpeg(processed, proc, 95);
        fclose(proc);
        printf("Saved processed cell: debug_processed.jpg\n");
    }
    
    // Cleanup
    gdImageDestroy(cell_img);
    gdImageDestroy(processed);
}

int main() {
    const char* image_path = "../data/images/level_1_binarized.png";
    
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
    
    printf("=== Debug Cell Extraction ===\n");
    printf("Image: %dx%d, TrueColor: %s\n", 
           gdImageSX(img), gdImageSY(img),
           gdImageTrueColor(img) ? "Yes" : "No");
    
    // Test first few pixels of image
    printf("\nFirst few pixels of original image:\n");
    for (int y = 0; y < 5; y++) {
        printf("Row %d: ", y);
        for (int x = 0; x < 10; x++) {
            int pixel = gdImageGetPixel(img, x, y);
            printf("%3d ", pixel);
        }
        printf("...\n");
    }
    
    // Detect grid
    BoundingBox grid = find_grid_by_projection(img);
    printf("\nGrid: x=%d, y=%d, w=%d, h=%d\n", 
           grid.x, grid.y, grid.width, grid.height);
    
    // Extract grid letters to get cell positions
    int rows, cols;
    LetterData** letters = extract_grid_letters(img, grid, &rows, &cols);
    
    if (letters && rows > 0 && cols > 0) {
        printf("\nGrid: %d x %d cells\n", rows, cols);
        
        // Test first cell (0,0)
        if (letters[0][0].letter_img) {
            printf("\nFirst cell bounding box: x=%d, y=%d, w=%d, h=%d\n",
                   letters[0][0].bbox.x, letters[0][0].bbox.y,
                   letters[0][0].bbox.width, letters[0][0].bbox.height);
            
            // Debug this specific cell
            test_single_cell(img, 
                           letters[0][0].bbox.x, 
                           letters[0][0].bbox.y,
                           letters[0][0].bbox.width,
                           letters[0][0].bbox.height);
            
            // Save the extracted letter image
            FILE* out = fopen("../data/grid/cells/debug_extracted.png", "wb");
            if (out) {
                gdImagePng(letters[0][0].letter_img, out);
                fclose(out);
                printf("Saved extracted letter: debug_extracted.png\n");
            }
        }
        
        // Cleanup
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