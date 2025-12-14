#include "rotation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

// STB Image implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void create_directories() {
    mkdir("data", 0755);
    mkdir("data/input", 0755);
    mkdir("data/images", 0755);
    mkdir("data/grid", 0755);
    mkdir("data/grid/cells", 0755);
    mkdir("data/wordlist", 0755);
    mkdir("data/wordlist/cells", 0755);
    mkdir("data/word_letters", 0755);
    mkdir("data/debug", 0755);
    mkdir("data/rotation_test", 0755);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("ERROR: not enough arguments\n");
        printf("Usage: %s <input_image>\n", argv[0]);
        printf("Example: %s wordsearch.jpg\n", argv[0]);
        return 1;
    }	

    printf("Word Search Solver - Complete Rotation Detection\n");
    printf("================================================\n");

    // Create output directories
    create_directories();
    
    // Step 1: Load image
    char input_file[256];
    snprintf(input_file, sizeof(input_file), "data/input/%s", argv[1]);
    
    printf("\nStep 1: Loading image\n");
    printf("-------------------\n");
    printf("Loading: %s\n", input_file);
    
    int width, height, channels;
    unsigned char* img = stbi_load(input_file, &width, &height, &channels, 0);
    if (!img) {
        printf("Error: Could not load image %s\n", input_file);
        printf("Make sure:\n");
        printf("  1. The file exists in data/input/ directory\n");
        printf("  2. You have read permissions\n");
        printf("  3. It's a supported image format (PNG, JPG, BMP)\n");
        return 1;
    }
    
    printf("Success: %dx%d pixels, %d channels\n", width, height, channels);
    
    // Allocate memory for processing
    unsigned char* bw_visual = NULL;
    unsigned char* rotation_buffer = NULL;
    
    bw_visual = (unsigned char*)malloc(width * height);
    
    if (!bw_visual) {
        printf("Error: Memory allocation failed\n");
        goto cleanup;
    }
   
    // Step 2: Preprocess image (simple noise removal only)
	printf("\nStep 2: Simple Noise Removal\n");
	printf("----------------------------\n");

	printf("Applying minimal noise removal (preserves all text)...\n");
	preprocess_ocr(img, bw_visual, width, height, channels);

	// Save the cleaned image
	stbi_write_png("data/images/01_cleaned.png",
        	width, height, 1, bw_visual, width);
	printf("Saved cleaned image: data/images/01_cleaned.png\n");
    
    // ========== COMPLETE ROTATION DETECTION & CORRECTION ==========
    printf("\nStep 3: Rotation Detection & Correction\n");
    printf("----------------------------------------\n");
    
    // Save visual test images for debugging
    printf("Saving test images for rotation analysis...\n");
    mkdir("data/rotation_test", 0755);
    
    unsigned char* test_buffer = malloc(width * height);
    if (test_buffer) {
        // Test a few angles
        double test_angles[] = {-15.0, 0.0, 15.0};
        for (int i = 0; i < 3; i++) {
            double angle = test_angles[i];
            rotate_image(bw_visual, test_buffer, width, height, angle);
            
            char filename[256];
            snprintf(filename, sizeof(filename), 
                     "data/rotation_test/test_%+06.1fdeg.png", angle);
            
            stbi_write_png(filename, width, height, 1, test_buffer, width);
        }
        free(test_buffer);
        printf("Saved test images to data/rotation_test/\n");
    }
    
    // Find the optimal rotation angle
    printf("\nDetecting optimal rotation angle...\n");
    double detected_rotation = find_best_angle(bw_visual, width, height);
    
    printf("\n=== CORRECTION DECISION ===\n");
    printf("Detected text orientation: %.2f°\n", detected_rotation);
    printf("(Positive = counter-clockwise, Negative = clockwise)\n");
    
    // Calculate correction angle
    double correction_angle = -detected_rotation;
    
    // Conservative rotation application
    if (fabs(correction_angle) < 0.5) {
        printf("\n✓ Image is already properly oriented (tilt < 0.5°)\n");
        printf("  Skipping rotation to avoid unnecessary processing artifacts\n");
        rotation_buffer = bw_visual;
        printf("  Using cleaned image for further processing\n");
    } 
    else if (fabs(correction_angle) < 2.0) {
        printf("\n⚠ Minor tilt detected (%.2f°)\n", correction_angle);
        printf("  Rotation is optional. Using cleaned image.\n");
        rotation_buffer = bw_visual;
    } 
    else {
        printf("\n✗ Significant rotation needed: %.2f°\n", correction_angle);
        printf("  Applying rotation correction...\n");
        
        // Allocate buffer for rotated image
        rotation_buffer = malloc(width * height);
        if (!rotation_buffer) {
            printf("Error: Could not allocate rotation buffer\n");
            goto cleanup;
        }
        
        // Clear the buffer first (white background)
        memset(rotation_buffer, 255, width * height);
        
        // Apply the rotation correction
        printf("  Rotating image by %.2f degrees...\n", correction_angle);
        rotate_image(bw_visual, rotation_buffer, width, height, correction_angle);
        
        // Save the corrected image
        char corrected_filename[256];
        snprintf(corrected_filename, sizeof(corrected_filename),
                 "data/images/treated_image.png");
        stbi_write_png(corrected_filename, width, height, 1, rotation_buffer, width);
        printf("  ✓ Saved corrected image: %s\n", corrected_filename);
    }
    
    // Note: The rest of your code (grid detection, word list, etc.) goes here
    // You'll need to convert the rotation_buffer to BinaryImage format
    // and continue with your pipeline
    
    printf("\nPreprocessing complete! Ready for grid detection.\n");
    
cleanup:
    // Cleanup
    printf("\nCleaning up resources...\n");
    
    if (bw_visual && bw_visual != rotation_buffer) {
        free(bw_visual);
        printf("  Freed cleaned image buffer\n");
    }
    
    if (rotation_buffer && rotation_buffer != bw_visual) {
        free(rotation_buffer);
        printf("  Freed rotation buffer\n");
    }
    
    if (img) {
        stbi_image_free(img);
        printf("  Freed original image\n");
    }

    printf("\nProgram finished.\n");
    return 0;
}
