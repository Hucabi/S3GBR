// tests/diagnostic.c
#include <gd.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE* fp = fopen("../data/images/level_1_binarized.png", "rb");
    if (!fp) {
        printf("Can't open image\n");
        return 1;
    }
    
    gdImagePtr img = gdImageCreateFromPng(fp);
    fclose(fp);
    
    if (!img) {
        printf("Failed to load image\n");
        return 1;
    }
    
    printf("Image: %dx%d\n", gdImageSX(img), gdImageSY(img));
    printf("TrueColor: %s\n", gdImageTrueColor(img) ? "Yes" : "No");
    
    // Check a range of pixels to see what's there
    printf("\n=== Scanning for black pixels near grid start (184,18) ===\n");
    
    // Check around the grid start
    for (int y = 15; y < 25; y++) {
        printf("Row %3d: ", y);
        for (int x = 180; x < 190; x++) {
            int pixel = gdImageGetPixel(img, x, y);
            if (gdImageTrueColor(img)) {
                int r = gdImageRed(img, pixel);
                printf("%3d ", r);
            } else {
                printf("%3d ", pixel);
            }
        }
        printf("...\n");
    }
    
    // Check if black is 0 or 255
    printf("\n=== Checking pixel values at borders ===\n");
    // Top-left pixel
    int tl = gdImageGetPixel(img, 0, 0);
    printf("Top-left (0,0): %d\n", tl);
    
    // Check a known border point
    printf("Border at (184,18): %d\n", gdImageGetPixel(img, 184, 18));
    printf("Border at (185,18): %d\n", gdImageGetPixel(img, 185, 18));
    
    // Check inside what should be a cell
    printf("Inside at (185,19): %d\n", gdImageGetPixel(img, 185, 19));
    printf("Inside at (200,30): %d\n", gdImageGetPixel(img, 200, 30));
    
    // Check letter area - scan for black pixels
    printf("\n=== Looking for black pixels in first 100x100 area ===\n");
    for (int y = 0; y < 100; y++) {
        for (int x = 0; x < 100; x++) {
            int pixel = gdImageGetPixel(img, x, y);
            if (pixel == 0) {  // Found black pixel
                printf("Black at (%d,%d)\n", x, y);
            }
        }
    }
    
    gdImageDestroy(img);
    return 0;
}