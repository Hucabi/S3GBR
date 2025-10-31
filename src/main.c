#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "image_loader.h"
#include "image_utils.h"
#include "grid_detection.h"
#include "word_list_detection.h"

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
}

// Convert preprocessing output to BinaryImage format
BinaryImage* convert_to_binary_image(unsigned char* bw_visual, int width, int height) {
	BinaryImage* binary = binary_image_create(width, height);
    
    	// Convert from 0/255 to 0/1 format (invert if needed)
    	for (int i = 0; i < width * height; i++) {
        	// In preprocessing: 0=black, 255=white
        	// In BinaryImage: 0=black, 255=white (same format)
        	binary->data[i] = bw_visual[i];
    	}
    
    	return binary;
}

// Rotation functions (from your friend's code)
void rotate_90(unsigned char* src, unsigned char* dest, int width, int height) {
    	for (int y = 0; y < height; y++)
        	for (int x = 0; x < width; x++)
            		dest[x * height + (height - 1 - y)] = src[y * width + x];
}

void rotate_180(unsigned char* src, unsigned char* dest, int width, int height) {
    	for (int y = 0; y < height; y++)
        	for (int x = 0; x < width; x++)
            		dest[(height - 1 - y) * width + (width - 1 - x)] = src[y * width + x];
}

void rotate_270(unsigned char* src, unsigned char* dest, int width, int height) {
    	for (int y = 0; y < height; y++)
        	for (int x = 0; x < width; x++)
            	dest[(width - 1 - x) * height + y] = src[y * width + x];
}

int main() {
    	printf("Word Search Solver - Starting...\n");
    
    	// Create output directories
    	create_directories();
    
    	// Step 1: Load and preprocess image
    	const char* input_file = "data/input/level_1_image_1.jpg";
    	printf("Loading image: %s\n", input_file);
    
    	int width, height, channels;
    	unsigned char* img = stbi_load(input_file, &width, &height, &channels, 0);
    	if (!img) {
        	printf("Error: Could not load image %s\n", input_file);
        	return 1;
    	}
    
    	printf("Image loaded: %dx%d (%d channels)\n", width, height, channels);
    
    	unsigned char* gray = NULL;
    	unsigned char* bw_logic = NULL;
    	unsigned char* bw_visual = NULL;
    	unsigned char* rotation_buffer = NULL;
    	BinaryImage* binary_img = NULL;
    	GridCells grid = {0, 0, 0, NULL};
    	int angle = 0;
	WordList word_list = {0, NULL, NULL, {0,0,0,0}};
    	int new_width = width, new_height = height;
	
	// Preprocessing
    	gray = (unsigned char*)malloc(width * height);
	bw_logic = (unsigned char*)malloc(width * height);
    	bw_visual = (unsigned char*)malloc(width * height);
    
    	if (!gray || !bw_logic || !bw_visual) {
        	printf("Error: Memory allocation failed\n");
        	goto cleanup;
    	}
    
    	// Convert to grayscale and binarize
    	rgb_to_gray(img, gray, width, height, channels);
    	binarize(gray, bw_logic, width, height);
    
    	// Convert 0/1 → 0/255 for visualization
    	for (int i = 0; i < width * height; i++) {
        	bw_visual[i] = bw_logic[i] ? 0 : 255;
    	}
    
    	// Save binarized image
    	stbi_write_png("data/images/level_1_binarized.png", width, height, 1, bw_visual, width);
    	printf("Saved binarized image as data/images/level_1_binarized.png\n");
    
    	// Step 2: Optional rotation
    	printf("Enter rotation angle (0, 90, 180, 270): ");
    	if (scanf("%d", &angle) != 1) {
    		printf("Invalid input. Using 0 (no rotation).\n");
    		angle = 0;}
    
    	unsigned char* rotated = bw_visual;
    
    	if (angle != 0) {
        	rotation_buffer = malloc(width * height);
		if (!rotation_buffer) {
            		printf("Error: Memory allocation for rotation failed\n");
            		goto cleanup;
        	}
        	switch(angle) {
            		case 90:
                		rotate_90(bw_visual, rotation_buffer, width, height);
                		new_width = height; 
                		new_height = width;
                		rotated = rotation_buffer;
                		break;
            		case 180:
                		rotate_180(bw_visual, rotation_buffer, width, height);
                		rotated = rotation_buffer;
                		break;
            		case 270:
                		rotate_270(bw_visual, rotation_buffer, width, height);
                		new_width = height; 
                		new_height = width;
                		rotated = rotation_buffer;
                		break;
            		default:
                		printf("⚠️Invalid angle. Skipping rotation.\n");
                		break;
        	}
        
        	if (rotation_buffer) {
            		stbi_write_png("data/images/rotated.png", new_width, new_height, 1, rotated, new_width);
            		printf("Rotated image saved as data/images/rotated.png\n");
        	}
    	}
    
    	// Step 3: Convert to BinaryImage format for detection
    	printf("Converting to BinaryImage format...\n");
   	binary_img = convert_to_binary_image(rotated, new_width, new_height);
    
    	// Step 4: Grid detection
    	printf("Detecting grid...\n");
    	grid = detect_grid_from_image(binary_img);
    	printf("Grid detected: %d x %d (%d cells)\n", grid.rows, grid.cols, grid.count);
    
    	// Save grid visualization
    	visualize_grid_detection(binary_img, grid,
			"data/images/grid_detection.png");
    	printf("Grid visualization saved as data/images/grid_detection.png\n");
    
    	// Save grid cells
    	int saved_cells = save_all_cells_binary(binary_img, grid,
			"data/grid/cells");
    	printf("Saved %d grid cells to data/grid/cells/\n", saved_cells);
    
    	// Step 5: Word list detection
    	printf("Detecting word list...\n");
    	word_list = find_word_list(binary_img, grid);
    	printf("Word list detected: %d words\n", word_list.count);
    
    	// Save word list visualization
    	visualize_word_list(binary_img, word_list,
			"data/images/word_list_detection.png");
    
    	// Save word list cells
    	save_word_list_cells(binary_img, word_list, "data");
    
    	// Step 6: Extract letters from words
   	save_word_letters(binary_img, word_list, "data");
    
    	// Free allocated memory from detection
    	word_list_free(&word_list);
    
    	printf("Program completed successfully.\n");

cleanup:
    	// Cleanup
    	if (gray) free(gray);
    	if (bw_logic) free(bw_logic);
    	if (bw_visual) free(bw_visual);
    	if (rotation_buffer) free(rotation_buffer);
	
	if (binary_img) binary_image_free(binary_img);

   	if (grid.rects) grid_cells_free(&grid);
    	if (word_list.words) word_list_free(&word_list);
	
    
    	if (img) stbi_image_free(img);

    	return 0;
}
