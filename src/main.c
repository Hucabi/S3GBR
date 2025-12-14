#include "rotation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include "image_loader.h"
#include "image_utils.h"
#include "grid_detection.h"
#include "word_list_detection.h"

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

// Convert preprocessing output to BinaryImage format
BinaryImage* convert_to_binary_image(unsigned char* bw_visual,
		int width,
		int height) {
	BinaryImage* binary = binary_image_create(width, height);
    
    	for (int i = 0; i < width * height; i++) {
        	binary->data[i] = bw_visual[i];
    	}
    	
    	return binary;
}

// Save visual debug images for rotation
void save_visual_rotation_tests(unsigned char* image, int width, int height) {
    printf("\n=== SAVING VISUAL ROTATION TESTS ===\n");
    
    unsigned char* buffer = malloc(width * height);
    if (!buffer) return;
    
    // Save several test rotations for visual inspection
    double test_angles[] = {-30.0, -15.0, 0.0, 15.0, 30.0, 90.0, 180.0};
    
    for (int i = 0; i < sizeof(test_angles)/sizeof(test_angles[0]); i++) {
        double angle = test_angles[i];
        rotate_image(image, buffer, width, height, angle);
        
        char filename[256];
        snprintf(filename, sizeof(filename), 
                 "data/rotation_test/visual_%+06.1fdeg.png", angle);
        
        stbi_write_png(filename, width, height, 1, buffer, width);
        printf("Saved: %s\n", filename);
    }
    
    free(buffer);
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
    unsigned char* gray = NULL;
    unsigned char* bw_logic = NULL;
    unsigned char* bw_visual = NULL;
    unsigned char* rotation_buffer = NULL;
    BinaryImage* binary_img = NULL;
    GridCells grid = {0, 0, 0, NULL};
    WordList word_list = {0, NULL, NULL, {0,0,0,0}};
    unsigned char* rotated = NULL;
    
    gray = (unsigned char*)malloc(width * height);
    bw_logic = (unsigned char*)malloc(width * height);
    bw_visual = (unsigned char*)malloc(width * height);
    
    if (!gray || !bw_logic || !bw_visual) {
        printf("Error: Memory allocation failed\n");
        goto cleanup;
    }
    
    // Step 2: Preprocess image
    printf("\nStep 2: Preprocessing\n");
    printf("-------------------\n");
    printf("Converting to grayscale...\n");
    rgb_to_gray(img, gray, width, height, channels);
    
    printf("Binarizing image...\n");
    binarize(gray, bw_logic, width, height);
    
    // Convert 0/1 → 0/255 for visualization
    for (int i = 0; i < width * height; i++) {
        bw_visual[i] = bw_logic[i] ? 0 : 255;  // 0=black, 255=white
    }
    
    // Save binarized image
    stbi_write_png("data/images/01_binarized.png",
            width, height, 1, bw_visual, width);
    printf("Saved: data/images/01_binarized.png\n");
    
    // ========== COMPLETE ROTATION DETECTION & CORRECTION ==========
    printf("\nStep 3: Rotation Detection & Correction\n");
    printf("----------------------------------------\n");
    
    // Save visual test images for debugging
    save_visual_rotation_tests(bw_visual, width, height);
    
    // Use the test function to see what different angles look like
    save_rotation_test_images(bw_visual, width, height);
    
    // Find the optimal rotation angle
    printf("\nDetecting optimal rotation angle...\n");
    double detected_rotation = find_best_angle(bw_visual, width, height);
    
    printf("\n=== CORRECTION DECISION ===\n");
    printf("Detected text orientation: %.2f°\n", detected_rotation);
    printf("(Positive = counter-clockwise, Negative = clockwise)\n");
    
    // Calculate correction angle
    // If text is rotated +X degrees, we need to rotate -X degrees to make it horizontal
    double correction_angle = -detected_rotation;
    
    // Conservative rotation application - only rotate if significant
    if (fabs(correction_angle) < 0.5) {
        printf("\n✓ Image is already properly oriented (tilt < 0.5°)\n");
        printf("  Skipping rotation to avoid unnecessary processing artifacts\n");
        rotated = bw_visual;
        printf("  Using original binarized image for further processing\n");
    } 
    else if (fabs(correction_angle) < 2.0) {
        printf("\n⚠ Minor tilt detected (%.2f°)\n", correction_angle);
        printf("  Rotation is optional. Using original to avoid blurring.\n");
        printf("  If OCR accuracy is poor, try manually rotating by %.2f°\n", correction_angle);
        rotated = bw_visual;
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
        rotated = rotation_buffer;
        
        // Save the corrected image
        char corrected_filename[256];
        snprintf(corrected_filename, sizeof(corrected_filename),
                 "data/images/02_corrected_%+.1fdeg.png", correction_angle);
        stbi_write_png(corrected_filename, width, height, 1, rotated, width);
        printf("  ✓ Saved corrected image: %s\n", corrected_filename);
        
        // Also save with standard name for pipeline
        stbi_write_png("data/images/02_corrected.png", width, height, 1, rotated, width);
        printf("  ✓ Saved as: data/images/02_corrected.png\n");
        
        // Save a comparison image showing before/after
        if (width > 0 && height > 0) {
            // Create side-by-side comparison for debugging
            int comparison_width = width * 2;
            unsigned char* comparison = malloc(comparison_width * height);
            if (comparison) {
                // Left side: original
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        comparison[y * comparison_width + x] = bw_visual[y * width + x];
                    }
                }
                // Right side: corrected
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        comparison[y * comparison_width + width + x] = rotated[y * width + x];
                    }
                }
                
                stbi_write_png("data/images/02_rotation_comparison.png", 
                              comparison_width, height, 1, comparison, comparison_width);
                free(comparison);
                printf("  ✓ Saved comparison: data/images/02_rotation_comparison.png\n");
            }
        }
    }
    // ========== END ROTATION SECTION ==========
    
    // Step 4: Convert to BinaryImage format
    printf("\nStep 4: Grid Detection\n");
    printf("---------------------\n");
    printf("Converting to BinaryImage format...\n");
    binary_img = convert_to_binary_image(rotated, width, height);
    
    // Detect grid
    printf("Detecting word search grid...\n");
    grid = detect_grid_from_image(binary_img);
    printf("Grid detected: %d rows × %d cols (%d total cells)\n", 
           grid.rows, grid.cols, grid.count);
    
    if (grid.rows == 0 || grid.cols == 0) {
        printf("WARNING: No grid detected!\n");
        printf("Trying fallback with original image...\n");
        
        // Clean up and try with original
        binary_image_free(binary_img);
        binary_img = convert_to_binary_image(bw_visual, width, height);
        grid = detect_grid_from_image(binary_img);
        printf("Grid with original image: %d × %d\n", grid.rows, grid.cols);
    }
    
    if (grid.rows > 0 && grid.cols > 0) {
        // Save grid visualization
        visualize_grid_detection(binary_img, grid,
                "data/images/03_grid_detection.png");
        printf("Saved: data/images/03_grid_detection.png\n");
        
        // Save individual cells
        int saved_cells = save_all_cells_binary(binary_img, grid,
                "data/grid/cells");
        printf("Saved %d grid cells to data/grid/cells/\n", saved_cells);
    } else {
        printf("ERROR: Still no grid detected. Cannot proceed.\n");
        goto cleanup;
    }
    
    // Step 5: Word list detection
    printf("\nStep 5: Word List Detection\n");
    printf("---------------------------\n");
    printf("Detecting word list area...\n");
    word_list = find_word_list(binary_img, grid);
    printf("Words detected: %d\n", word_list.count);
    
    if (word_list.count > 0) {
        // Visualize word list
        visualize_word_list(binary_img, word_list,
                "data/images/04_word_list.png");
        printf("Saved: data/images/04_word_list.png\n");
        
        // Save word list cells
        save_word_list_cells(binary_img, word_list, "data");
        
        // Extract letters from words
        save_word_letters(binary_img, word_list, "data");
        printf("Word letters extracted to data/word_letters/\n");
        
        // Display detected words
        printf("\nDetected words:\n");
        for (int i = 0; i < word_list.count && i < 10; i++) {
            printf("  Word %d: [%s]\n", i+1, word_list.words[i]);
        }
        if (word_list.count > 10) {
            printf("  ... and %d more\n", word_list.count - 10);
        }
    } else {
        printf("WARNING: No words detected in word list area\n");
    }
    
    // Clean up word list
    word_list_free(&word_list);
    
cleanup:
    // Cleanup all allocated memory
    printf("\nCleaning up resources...\n");
    
    if (gray) {
        free(gray);
        printf("  Freed grayscale buffer\n");
    }
    
    if (bw_logic) {
        free(bw_logic);
        printf("  Freed binary logic buffer\n");
    }
    
    if (bw_visual && bw_visual != rotated) {
        // Only free if it's not pointing to the rotated buffer
        free(bw_visual);
        printf("  Freed binary visual buffer\n");
    }
    
    if (rotation_buffer) {
        free(rotation_buffer);
        printf("  Freed rotation buffer\n");
    }
    
    if (binary_img) {
        binary_image_free(binary_img);
        printf("  Freed binary image structure\n");
    }

    if (grid.rects) {
        grid_cells_free(&grid);
        printf("  Freed grid rectangles\n");
    }
    
    if (img) {
        stbi_image_free(img);
        printf("  Freed original image\n");
    }

    printf("\nProgram finished.\n");
    return 0;
}
