#include "localization.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <image.png>\n", argv[0]);
        return 1;
    }
    
    // Load binarized image
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
    
    // Process image
    ExtractionResult* result = localize_and_extract(img);
    
    // Save debug outputs
    save_debug_images(result, "debug");
    
    // Cleanup
    free_extraction_result(result);
    gdImageDestroy(img);
    
    return 0;
}