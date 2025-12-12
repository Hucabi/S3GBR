#include "localization.h"
#include <stdio.h>

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
        printf("Failed to load image\n");
        return 1;
    }
    
    printf("Testing grid detection on: %s\n", argv[1]);
    printf("Image: %dx%d pixels\n", gdImageSX(img), gdImageSY(img));
    printf("TrueColor: %s\n", gdImageTrueColor(img) ? "Yes" : "No");
    
    // Test projection functions
    int* h_proj = compute_horizontal_projection(img);
    int* v_proj = compute_vertical_projection(img);
    
    // Find potential borders
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    int border_threshold = width * 0.7;
    
    printf("\nLooking for horizontal borders (threshold: %d pixels):\n", border_threshold);
    for (int y = 0; y < height; y++) {
        if (h_proj[y] > border_threshold) {
            printf("  Border at row %d: %d black pixels\n", y, h_proj[y]);
        }
    }
    
    border_threshold = height * 0.7;
    printf("\nLooking for vertical borders (threshold: %d pixels):\n", border_threshold);
    for (int x = 0; x < width; x++) {
        if (v_proj[x] > border_threshold) {
            printf("  Border at column %d: %d black pixels\n", x, v_proj[x]);
        }
    }
    
    // Try grid detection
    printf("\n=== Testing grid detection ===\n");
    BoundingBox grid = find_grid_by_projection(img);
    printf("Grid found: x=%d, y=%d, w=%d, h=%d\n", 
           grid.x, grid.y, grid.width, grid.height);
    
    // Save a debug image with the grid highlighted
    if (grid.width > 0 && grid.height > 0) {
        // Create a copy of the image
        gdImagePtr debug = gdImageCreate(gdImageSX(img), gdImageSY(img));
        gdImageCopy(debug, img, 0, 0, 0, 0, gdImageSX(img), gdImageSY(img));
        
        // Draw red rectangle around grid
        int red = gdImageColorAllocate(debug, 255, 0, 0);
        gdImageRectangle(debug, grid.x, grid.y, 
                        grid.x + grid.width, grid.y + grid.height, red);
        
        // Save debug image
        FILE* out = fopen("debug_grid.png", "wb");
        gdImagePng(debug, out);
        fclose(out);
        gdImageDestroy(debug);
        
        printf("Debug image saved as: debug_grid.png\n");
    }
    
    // Cleanup
    free(h_proj);
    free(v_proj);
    gdImageDestroy(img);
    
    return 0;
}