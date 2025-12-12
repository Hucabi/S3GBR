#include <gd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main() {
    printf("=== Working Extraction Test ===\n");
    
    // Load image
    FILE* fp = fopen("../data/images/level_1_binarized.png", "rb");
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
    
    printf("Image: %dx%d\n", gdImageSX(img), gdImageSY(img));
    
    // Create output directory
    mkdir("../data/grid/cells", 0755);
    
    // MANUAL extraction - we know the grid layout from earlier tests
    // Grid: 16x16 cells, starts at (185,19)
    // Each cell is about 34x35 pixels (based on earlier calculations)
    
    int grid_start_x = 185;
    int grid_start_y = 19;
    int cell_width = 34;
    int cell_height = 35;
    int rows = 16;
    int cols = 16;
    
    printf("Extracting %dx%d grid cells...\n", rows, cols);
    
    int saved_count = 0;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int cell_x = grid_start_x + col * cell_width;
            int cell_y = grid_start_y + row * cell_height;
            
            // Create 32x32 output image
            gdImagePtr output = gdImageCreate(32, 32);
            
            // Fill with white (index 255)
            for (int y = 0; y < 32; y++) {
                for (int x = 0; x < 32; x++) {
                    gdImageSetPixel(output, x, y, 255);
                }
            }
            
            // Copy and scale letter from cell
            for (int y = 0; y < 32; y++) {
                for (int x = 0; x < 32; x++) {
                    // Map to source cell coordinates
                    int src_x = cell_x + (x * cell_width) / 32;
                    int src_y = cell_y + (y * cell_height) / 32;
                    
                    if (src_x < gdImageSX(img) && src_y < gdImageSY(img)) {
                        int color = gdImageGetPixel(img, src_x, src_y);
                        if (color == 0) { // Black pixel
                            gdImageSetPixel(output, x, y, 0);
                        }
                    }
                }
            }
            
            // Save as JPEG
            char filename[256];
            snprintf(filename, sizeof(filename), 
                    "../data/grid/cells/cell_%02d_%02d.jpg", row, col);
            
            FILE* out = fopen(filename, "wb");
            if (out) {
                // Convert to true color for JPEG
                gdImagePtr truecolor = gdImageCreateTrueColor(32, 32);
                for (int y = 0; y < 32; y++) {
                    for (int x = 0; x < 32; x++) {
                        int color = gdImageGetPixel(output, x, y);
                        if (color == 0) {
                            gdImageSetPixel(truecolor, x, y, 
                                          gdImageColorAllocate(truecolor, 0, 0, 0));
                        } else {
                            gdImageSetPixel(truecolor, x, y, 
                                          gdImageColorAllocate(truecolor, 255, 255, 255));
                        }
                    }
                }
                
                gdImageJpeg(truecolor, out, 95);
                fclose(out);
                gdImageDestroy(truecolor);
                saved_count++;
                
                if (saved_count <= 4) {
                    printf("Saved: %s\n", filename);
                }
            }
            
            gdImageDestroy(output);
        }
    }
    
    printf("Total saved: %d cells\n", saved_count);
    printf("Check: ls -la ../data/grid/cells/cell_00_00.jpg\n");
    
    gdImageDestroy(img);
    return 0;
}