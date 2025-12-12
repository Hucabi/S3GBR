#include <gd.h>
#include <stdio.h>
#include <stdlib.h>

// Remove the local definition and declare the external function
int* compute_horizontal_projection(gdImagePtr img);

void print_projection(int* proj, int length, int max_rows) {
    printf("Projection (first %d rows):\n", max_rows);
    for (int i = 0; i < max_rows && i < length; i++) {
        printf("Row %3d: %4d pixels\n", i, proj[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <image.png>\n", argv[0]);
        return 1;
    }
    
    // Load image
    FILE* fp = fopen(argv[1], "rb");
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
    
    printf("Image loaded successfully\n");
    printf("Colors Total: %d\n", gdImageColorsTotal(img));
    printf("Image size: %dx%d\n", gdImageSX(img), gdImageSY(img));
    
    // Test: Check first few pixel values
    printf("First pixel (0,0): %d\n", gdImageGetPixel(img, 0, 0));
    printf("Pixel (10,10): %d\n", gdImageGetPixel(img, 10, 10));
    printf("Pixel (100,100): %d\n", gdImageGetPixel(img, 100, 100));
    
    // Compute projection
    int* proj = compute_horizontal_projection(img);
    print_projection(proj, gdImageSY(img), 30);
    
    // Find where the grid starts (first row with significant black pixels)
    int grid_start_y = -1;
    for (int i = 0; i < gdImageSY(img); i++) {
        if (proj[i] > 100) {  // Threshold for border
            grid_start_y = i;
            break;
        }
    }
    printf("Grid likely starts at row: %d\n", grid_start_y);
    
    // Cleanup
    free(proj);
    gdImageDestroy(img);
    
    return 0;
}