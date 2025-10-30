#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "word_list_detection.h"
#include "grid_detection.h"
#include "stb_image_write.h"
#include <unistd.h>

// Find bounding box of the grid
Rectangle find_grid_bounding_box(GridCells grid) {
    	if (grid.count == 0) return (Rectangle){0,0,0,0};
    
    	int min_x = grid.rects[0].x;
    	int min_y = grid.rects[0].y;
    	int max_x = grid.rects[0].x + grid.rects[0].w;
    	int max_y = grid.rects[0].y + grid.rects[0].h;
    
    	for (int i = 1; i < grid.count; i++) {
        	if (grid.rects[i].x < min_x) min_x = grid.rects[i].x;
        	if (grid.rects[i].y < min_y) min_y = grid.rects[i].y;
        	if (grid.rects[i].x + grid.rects[i].w > max_x) max_x = grid.rects[i].x + grid.rects[i].w;
        	if (grid.rects[i].y + grid.rects[i].h > max_y) max_y = grid.rects[i].y + grid.rects[i].h;
    	}
    
    	return (Rectangle){min_x, min_y, max_x - min_x, max_y - min_y};
}

// Analyze text density in a region
double analyze_region_for_word_list(BinaryImage *img, Rectangle region) {
    	int black_pixels = 0;
    	int total_pixels = 0;
    
    	// Sample points to check if this is a grid (avoid dense grid areas)
    	int grid_like_patterns = 0;
    	
    	for (int y = region.y; y < region.y + region.h && y < img->height; y += 5) { // Sample every 5px
        	for (int x = region.x; x < region.x + region.w && x < img->width; x += 5) {
            		unsigned char* pixel = binary_pixel_at(img, x, y);
            		if (pixel && *pixel == 0) {
                		black_pixels++;
                
                		// Check if this looks like grid structure (many adjacent black pixels)
                	int adjacent_black = 0;
                	for (int dy = -1; dy <= 1; dy++) {
                    		for (int dx = -1; dx <= 1; dx++) {
                        		int nx = x + dx, ny = y + dy;
                        		if (nx >= region.x && nx < region.x + region.w && 
                            			ny >= region.y && ny < region.y + region.h) {
                            				unsigned char* neighbor = binary_pixel_at(img, nx, ny);
                            				if (neighbor && *neighbor == 0) {
                                				adjacent_black++;
                            				}
                        		}
                    		}
                	}
                	if (adjacent_black > 4) { // Dense pattern = likely grid
                    		grid_like_patterns++;
                	}
            	}
            	total_pixels++;
        	}
    	}
    
    	double density = (double)black_pixels / total_pixels;
    	double grid_score = (double)grid_like_patterns / (black_pixels + 1);
    
    	// Penalize regions that look like grids
    	return density * (1.0 - grid_score * 0.8);
}

// Find lines of text in a region
int find_text_lines(BinaryImage *img, Rectangle region, Rectangle **lines) {
    	// Horizontal projection to find text lines
    	int *h_proj = calloc(region.h, sizeof(int));
    
    	for (int y = 0; y < region.h; y++) {
        	for (int x = 0; x < region.w; x++) {
            		int orig_x = region.x + x;
            		int orig_y = region.y + y;
            		if (orig_x < img->width && orig_y < img->height) {
                		unsigned char* pixel = binary_pixel_at(img, orig_x, orig_y);
                		if (pixel && *pixel == 0) {
                    		h_proj[y]++;
                		}
            		}
        	}
    	}
    
    	// Find peaks in projection (text lines)
    	int line_count = 0;
    	int *line_positions = malloc(region.h * sizeof(int));
    
    	int threshold = 5; // Minimum black pixels to be considered a text line
    	int in_line = 0;
    	int line_start = 0;
    
    	for (int y = 0; y < region.h; y++) {
        	if (h_proj[y] > threshold) {
            		if (!in_line) {
                		in_line = 1;
                		line_start = y;
            		}
        	}
		else {
            		if (in_line) {
                		in_line = 0;
                		int line_center = line_start + (y - line_start) / 2;
                		line_positions[line_count++] = line_center;
            		}
        	}
    	}
    
    	// Create line rectangles
    	*lines = malloc(line_count * sizeof(Rectangle));
    	int line_height = 20; // Estimated line height
    
    	for (int i = 0; i < line_count; i++) {
        	int center_y = line_positions[i];
        	(*lines)[i] = (Rectangle){
            		region.x,
            		region.y + center_y - line_height/2,
            		region.w,
            		line_height
        		};
        
        	// Adjust boundaries
        	if ((*lines)[i].y < region.y) (*lines)[i].y = region.y;
        	if ((*lines)[i].y + (*lines)[i].h > region.y + region.h) {
            		(*lines)[i].h = (region.y + region.h) - (*lines)[i].y;
        	}
    	}
    
    	free(h_proj);
    	free(line_positions);
    	return line_count;
}

int find_text_lines_custom(BinaryImage *img, Rectangle region, Rectangle **lines, int threshold) {
    	// Horizontal projection of the region
    	int *h_proj = calloc(region.h, sizeof(int));
    
    	for (int y = 0; y < region.h; y++) {
        	for (int x = 0; x < region.w; x++) {
            		int orig_x = region.x + x;
            		int orig_y = region.y + y;
            		if (orig_x < img->width && orig_y < img->height) {
                		unsigned char* pixel = binary_pixel_at(img, orig_x, orig_y);
                		if (pixel && *pixel == 0) {
                    			h_proj[y]++;
                		}
            		}
        	}
    	}
    
    	// Find peaks in projection (text lines) with CUSTOM threshold
    	int line_count = 0;
    	int *line_positions = malloc(region.h * sizeof(int));
    
    	int in_line = 0;
    	int line_start = 0;
    
    	for (int y = 0; y < region.h; y++) {
        	if (h_proj[y] > threshold) { // Use custom threshold
            		if (!in_line) {
                		in_line = 1;
                		line_start = y;
            		}
        	}
		else {
            		if (in_line) {
                		in_line = 0;
                		int line_center = line_start + (y - line_start) / 2;
                		line_positions[line_count++] = line_center;
            		}
        	}
    	}
    
    	// Handle case where line ends at image boundary
    	if (in_line) {
        	int line_center = line_start + (region.h - line_start) / 2;
        	line_positions[line_count++] = line_center;
    	}
    
    	// Create line rectangles
    	*lines = malloc(line_count * sizeof(Rectangle));
    	int line_height = 25; // Slightly taller for word list
    
    	for (int i = 0; i < line_count; i++) {
        	int center_y = line_positions[i];
        	(*lines)[i] = (Rectangle){
            		region.x,
            		region.y + center_y - line_height/2,
            		region.w,
            		line_height
        		};
        
        	// Adjust boundaries
        	if ((*lines)[i].y < region.y) (*lines)[i].y = region.y;
        	if ((*lines)[i].y + (*lines)[i].h > region.y + region.h) {
          		(*lines)[i].h = (region.y + region.h) - (*lines)[i].y;
        	}
    	}
    
    	free(h_proj);
    	free(line_positions);
    	return line_count;
}

WordList find_word_list(BinaryImage *img, GridCells grid) {
   	WordList word_list = {0, NULL, NULL, {0,0,0,0}};
    
   	printf("Word list detection - Starting...\n");

    	// 1. Find the grid region
    	Rectangle grid_region = find_grid_bounding_box(grid);
    	printf("Grid region: x=%d, y=%d, w=%d, h=%d\n", 
           	grid_region.x, grid_region.y, grid_region.w, grid_region.h);
    
    	// 2. Calculate word list region based on grid position
    	int word_list_width = grid_region.x - 20; // Use space before grid starts
    	if (word_list_width < 100) word_list_width = 180; // Minimum width
    	Rectangle word_list_region = {
        	10,                            // x = slight margin from left
        	grid_region.y,                 // y = align with grid top
        	word_list_width - 10,          // w = space before grid
        	grid_region.h                  // h = same height as grid
    	};

    	printf("Using adaptive word list region: %dx%d at (%d,%d)\n",
           	word_list_region.w, word_list_region.h, word_list_region.x, word_list_region.y); 
    	
    	// 3. Check if this region actually has text
    	int black_pixels = 0;
   	int sample_points = 0;
    
    	for (int y = word_list_region.y; y < word_list_region.y + word_list_region.h; y += 3) {
        	for (int x = word_list_region.x; x < word_list_region.x + word_list_region.w; x += 3) {
            		if (x < img->width && y < img->height) {
                		unsigned char* pixel = binary_pixel_at(img, x, y);
                		if (pixel && *pixel == 0) {
                    			black_pixels++;
                		}
                		sample_points++;
            		}
        	}
    	}

   	double density = (double)black_pixels / sample_points;
    	printf("Word list region density: %.3f (%d black pixels in %d samples)\n", 
           	density, black_pixels, sample_points);

    	// 4. If there's text in this region, extract it
    	if (density > 0.005) { // Very low threshold just to check if there's any text
        	word_list.list_region = word_list_region;
        
        	Rectangle *text_lines = NULL;
         	int line_count = find_text_lines_custom(img, word_list_region, &text_lines, 2); // Threshold = 2
        
        	printf("Found %d text lines in word list region\n", line_count);
        
        	if (line_count > 0) {
            		word_list.count = line_count;
            		word_list.words = malloc(line_count * sizeof(char*));
            		word_list.word_rects = malloc(line_count * sizeof(Rectangle));
            
            		for (int i = 0; i < line_count; i++) {
                		char word_name[20];
                		snprintf(word_name, sizeof(word_name), "word_%d", i);
                		word_list.words[i] = malloc(strlen(word_name) + 1);
                		strcpy(word_list.words[i], word_name);
                
                		word_list.word_rects[i] = text_lines[i];
                
                		printf("Detected word %d at (%d,%d) %dx%d\n", i,
                       			word_list.word_rects[i].x, word_list.word_rects[i].y,
                       			word_list.word_rects[i].w, word_list.word_rects[i].h);
            		}
            	
        	}
		else {
            		printf("WARNING: Found text in region but couldn't detect lines. Use fallback\n");
            
			// Fallback: create evenly spaced lines
          		word_list.count = 9; // Default expected count
            		word_list.words = malloc(word_list.count * sizeof(char*));
            		word_list.word_rects = malloc(word_list.count * sizeof(Rectangle));
	
        		int line_height = word_list_region.h / (word_list.count + 1);
            		for (int i = 0; i < word_list.count; i++) {
                		char word_name[20];
                		snprintf(word_name, sizeof(word_name), "word_%d", i);
                		word_list.words[i] = malloc(strlen(word_name) + 1);
                		strcpy(word_list.words[i], word_name);

                		word_list.word_rects[i] = (Rectangle){
                    			word_list_region.x + 5,
                    			word_list_region.y + 10 + (i * line_height),
                    			word_list_region.w - 10,
                    			line_height - 5
                		};
            		}
        	}

		free(text_lines);
	}
	else {
        	printf("No text found in word list region (density too low: %.3f)\n", density);
    	}
    
    	printf("Word list detection - Completed (%d words found)\n", word_list.count);
    	return word_list;
}

void visualize_word_list(BinaryImage *original, WordList word_list, const char *output_path) {
    	if (word_list.count == 0) {
        	printf("No words to visualize\n");
        	return;
    	}
	
    	// Create RGB image
    	unsigned char *rgb_data = malloc(original->width * original->height * 3);
    
    	// Convert binary to RGB (white background, black text)
    	for (int i = 0; i < original->width * original->height; i++) {
        	unsigned char val = original->data[i];
        	rgb_data[i*3] = val;     // R
        	rgb_data[i*3+1] = val;   // G  
        	rgb_data[i*3+2] = val;   // B
    	}

    	// Draw word list region with semi-transparent blue overlay
    	Rectangle r = word_list.list_region;
    	for (int x = r.x; x < r.x + r.w && x < original->width; x++) {
        	for (int y = r.y; y < r.y + r.h && y < original->height; y++) {
            		int idx = (y * original->width + x) * 3;
            		// Blue overlay (blend with original)
            		rgb_data[idx] = (rgb_data[idx] + 100) / 2;      // R
            		rgb_data[idx+1] = (rgb_data[idx+1] + 150) / 2;  // G
            		rgb_data[idx+2] = 255;                          // B (full)
        	}
    	}

    	// Draw word bounding boxes in bright green
    	for (int i = 0; i < word_list.count; i++) {
        	Rectangle word_rect = word_list.word_rects[i];
        
        	// Draw top and bottom borders
        	for (int x = word_rect.x; x < word_rect.x + word_rect.w && x < original->width; x++) {
            		if (word_rect.y >= 0 && word_rect.y < original->height) {
                		int idx = (word_rect.y * original->width + x) * 3;
                		rgb_data[idx] = 0;      // R
                		rgb_data[idx+1] = 255;  // G
                		rgb_data[idx+2] = 0;    // B
            		}
            		if (word_rect.y + word_rect.h >= 0 && word_rect.y + word_rect.h < original->height) {
                		int idx = ((word_rect.y + word_rect.h) * original->width + x) * 3;
                		rgb_data[idx] = 0;      // R
                		rgb_data[idx+1] = 255;  // G
                		rgb_data[idx+2] = 0;    // B
            		}
        	}
        
        	// Draw left and right borders
        	for (int y = word_rect.y; y < word_rect.y + word_rect.h && y < original->height; y++) {
            		if (word_rect.x >= 0 && word_rect.x < original->width) {
                		int idx = (y * original->width + word_rect.x) * 3;
                		rgb_data[idx] = 0;      // R
                		rgb_data[idx+1] = 255;  // G
                		rgb_data[idx+2] = 0;    // B
            		}
            		if (word_rect.x + word_rect.w >= 0 && word_rect.x + word_rect.w < original->width) {
                		int idx = (y * original->width + (word_rect.x + word_rect.w)) * 3;
                		rgb_data[idx] = 0;      // R
                		rgb_data[idx+1] = 255;  // G
                		rgb_data[idx+2] = 0;    // B
            		}
        	}
    	}

	// Save as PNG using stb_image_write
    	int success = stbi_write_png(output_path, original->width, original->height, 3, rgb_data, original->width * 3);

    	if (success) {
        	printf("Word list visualization saved as %s\n", output_path);
    	}
	else {
        	printf("ERROR: Failed to save PNG: %s\n", output_path);
    	}

    	free(rgb_data);
}

void save_word_list_cells(BinaryImage *img, WordList word_list, const char *base_path) {
    	if (word_list.count == 0) {
        	printf("No word list cells to save\n");
        	return;
    	}
		
   	if (access(base_path, F_OK) == -1) {
        	printf("ERROR: Base directory does not exist: %s\n", base_path);
        	return;
    	}	
	
    	// Create the full directory path
    	char path[256];
    	snprintf(path, sizeof(path), "%s/wordlist/cells", base_path);
    
    	// Check if the target directory exists
	if (access(path, F_OK) == -1) {
        	printf("ERROR: Target directory does not exist: %s\n", path);
        	return;
    	}

	printf("Saving %d word list cells to %s/\n", word_list.count, path);
    
    	for (int i = 0; i < word_list.count; i++) {
        	char filename[512];
        
        	snprintf(filename, sizeof(filename), "%s/w%d.png", path, i);
        	
		Rectangle word_rect = word_list.word_rects[i];
        
        	// Create RGB image for the word cell
        	unsigned char *rgb_data = malloc(word_rect.w * word_rect.h * 3);
        
        	for (int y = 0; y < word_rect.h; y++) {
           	 	for (int x = 0; x < word_rect.w; x++) {
                		int orig_x = word_rect.x + x;
                		int orig_y = word_rect.y + y;
                		unsigned char pixel_val = 255; // Default white
                
                		if (orig_x < img->width && orig_y < img->height) {
                    			unsigned char* pixel = binary_pixel_at(img, orig_x, orig_y);
                    			pixel_val = pixel ? *pixel : 255;
                		}		
                
                		int idx = (y * word_rect.w + x) * 3;
                	rgb_data[idx] = pixel_val;     // R
                	rgb_data[idx+1] = pixel_val;   // G
                	rgb_data[idx+2] = pixel_val;   // B
            		}
        	}
        
        	// Save as PNG using stb_image_write
        	int success = stbi_write_png(filename, word_rect.w, word_rect.h, 3, rgb_data, word_rect.w * 3);
        
        	if (success) {
            		printf("Saved: %s\n", filename);
        	}
		else {
            	printf("Failed to save word image: %s\n", filename);
        	}
        
	free(rgb_data);
	}
}

// Similar to grid cells resize
BinaryImage* resize_image(BinaryImage *img, int target_width, int target_height) {
    	BinaryImage *resized = binary_image_create(target_width, target_height);
    	for (int y = 0; y < target_height; y++) {
        	for (int x = 0; x < target_width; x++) {
            		int src_x = (x * img->width) / target_width;
            		int src_y = (y * img->height) / target_height;
            		resized->data[y * target_width + x] = img->data[src_y * img->width + src_x];
        	}
    	}
    	return resized;
}

// Segment word into individual letters w vertical projection
int segment_word_letters(BinaryImage *word_img, Rectangle **letter_rects) {
    	if (word_img->width == 0 || word_img->height == 0) {
        	*letter_rects = NULL;
        	return 0;
    	}

    	// Compute vertical projection
    	int *v_proj = calloc(word_img->width, sizeof(int));
    	for (int x = 0; x < word_img->width; x++) {
        	for (int y = 0; y < word_img->height; y++) {
            		if (word_img->data[y * word_img->width + x] == 0) { // Black pixel
                		v_proj[x]++;
            		}
        	}
    	}

    	// Find letter boundaries
    	int *boundaries = malloc((word_img->width + 1) * sizeof(int));
    	int boundary_count = 0;

    	boundaries[boundary_count++] = 0;

    	// Find local minimum -> gaps between letters
    	int min_threshold = word_img->height * 0.1; // At least 10% of height should be black for a letter column
    	int gap_threshold = word_img->height * 0.05; // Less than 5% black pixels indicates a gap

    	int in_letter = 0;

    	for (int x = 0; x < word_img->width; x++) {
        	if (v_proj[x] > min_threshold) {
            		if (!in_letter) {
                		in_letter = 1;
            		}
        	}
		else if (v_proj[x] < gap_threshold) {
            		if (in_letter) {
                		in_letter = 0;
                		boundaries[boundary_count++] = x;
            		}
        	}
    	}

    	if (in_letter) {
        	boundaries[boundary_count++] = word_img->width;
    	}
	else if (boundary_count > 0 && boundaries[boundary_count-1] != word_img->width) {
        	boundaries[boundary_count++] = word_img->width;
    	}

    	// Create letter rectangles
    	int letter_count = boundary_count - 1;
    	if (letter_count <= 0) {
        	free(v_proj);
        	free(boundaries);
        	*letter_rects = NULL;
        	return 0;
    	}

    	*letter_rects = malloc(letter_count * sizeof(Rectangle));

    	for (int i = 0; i < letter_count; i++) {
        	int left = boundaries[i];
        	int right = boundaries[i + 1];

        	int padding = 1;
        	int x = (left > padding) ? left - padding : 0;
        	int w = right - left + 2 * padding;
        	if (x + w > word_img->width) {
            		w = word_img->width - x;
        	}

        	(*letter_rects)[i] = (Rectangle){x, 0, w, word_img->height};
    	}

    	free(v_proj);
    	free(boundaries);
    	return letter_count;
}

// Extract + save individual letters from word list
void save_word_letters(BinaryImage *img, WordList word_list, const char *base_path) {
    	if (word_list.count == 0) {
        	printf("No words to extract letters from\n");
        	return;
    	}

    	// Check if base directory exists
    	if (access(base_path, F_OK) == -1) {
        	printf("ERROR: Base directory does not exist: %s\n", base_path);
        	return;
    	}

    	char letters_base_path[512];
    	long unsigned int written = snprintf(letters_base_path, sizeof(letters_base_path), "%s/word_letters", base_path);
	if (written >= sizeof(letters_base_path)) {
        	printf("ERROR: Path too long: %s/word_letters\n", base_path);
        	return;
    	}


    	// Check if target directory exists
    	if (access(letters_base_path, F_OK) == -1) {
        	printf("ERROR: Word letters directory does not exist: %s\n", letters_base_path);
        	printf("Please create: mkdir -p %s\n", letters_base_path);
        	return;
    	}

    	printf("Extracting letters from %d words to %s/\n", word_list.count, letters_base_path);

    	for (int word_idx = 0; word_idx < word_list.count; word_idx++) {
        	// Create word directory
        	char word_dir[512];
        	written = snprintf(word_dir, sizeof(word_dir), "%s/word_%d", letters_base_path, word_idx);
		if (written >= sizeof(word_dir)) {
            		printf("Warning: Path too long for word directory, skipping word %d\n", word_idx);
            		continue;
        	}

        	if (access(word_dir, F_OK) == -1) {
            		char command[1024];
            		snprintf(command, sizeof(command), "mkdir -p %s", word_dir);

			int result = system(command);
            		if (result != 0) {
                		printf("Warning: Failed to create directory %s\n", word_dir);
                		continue;
            		}
        	}

        	// Extract the word image from main image
        	Rectangle word_rect = word_list.word_rects[word_idx];
        	BinaryImage *word_img = binary_image_create(word_rect.w, word_rect.h);

        	for (int y = 0; y < word_rect.h; y++) {
            		for (int x = 0; x < word_rect.w; x++) {
                		int orig_x = word_rect.x + x;
                		int orig_y = word_rect.y + y;
                		if (orig_x < img->width && orig_y < img->height) {
                    			unsigned char* pixel = binary_pixel_at(img, orig_x, orig_y);
                    			word_img->data[y * word_rect.w + x] = pixel ? *pixel : 255;
                		}
				else {
                   	 		word_img->data[y * word_rect.w + x] = 255; // White if out of bounds
                		}
            		}
        	}

        	// Segment word into individual letters
        	Rectangle *letter_rects = NULL;
        	int letter_count = segment_word_letters(word_img, &letter_rects);

        	printf("Word %d: segmented into %d letters\n", word_idx, letter_count);

        	// Save each letter
        	for (int letter_idx = 0; letter_idx < letter_count; letter_idx++) {
            		Rectangle letter_rect = letter_rects[letter_idx];

            	// Extract letter image from word image
            	BinaryImage *letter_img = binary_image_create(letter_rect.w, letter_rect.h);

            	for (int y = 0; y < letter_rect.h; y++) {
                	for (int x = 0; x < letter_rect.w; x++) {
                    		int word_x = letter_rect.x + x;
                    		int word_y = letter_rect.y + y;
                    		if (word_x < word_img->width && word_y < word_img->height) {
                        		letter_img->data[y * letter_rect.w + x] =
                            		word_img->data[word_y * word_img->width + word_x];
                    		}
				else {
                        		letter_img->data[y * letter_rect.w + x] = 255; // White
                    		}
                	}
            	}

            	// Resize to 28x28 (standard for OCR)
            	BinaryImage *resized_letter = resize_image(letter_img, 28, 28);

            	// Save as PNG
            	char filename[512];
            	written = snprintf(filename, sizeof(filename), "%s/letter_%d.png", word_dir, letter_idx);
		if (written >= sizeof(filename)) {
                	printf("Warning: Filename too long for letter %d in word %d, skipping\n", letter_idx, word_idx);
                	binary_image_free(letter_img);
                	binary_image_free(resized_letter);
                	continue;
            	}

            	// Convert to RGB for saving
            	unsigned char *rgb_data = malloc(28 * 28 * 3);
            	for (int i = 0; i < 28 * 28; i++) {
                	unsigned char val = resized_letter->data[i];
                	rgb_data[i*3] = val;
                	rgb_data[i*3+1] = val;
                	rgb_data[i*3+2] = val;
            	}

            	int success = stbi_write_png(filename, 28, 28, 3, rgb_data, 28 * 3);

            	if (success) {
                	//printf("  Saved: %s\n", filename);
            	}
		else {
                	printf("  Failed to save letter: %s\n", filename);
            	}

            	free(rgb_data);
            	binary_image_free(letter_img);
            	binary_image_free(resized_letter);
        }

        // Cleanup
        if (letter_rects) free(letter_rects);
        	binary_image_free(word_img);
    	}

    	printf("Letter extraction completed\n");
}

void word_list_free(WordList *word_list) {
    	if (word_list && word_list->words) {
        	for (int i = 0; i < word_list->count; i++) {
            		free(word_list->words[i]);
        	}
        	free(word_list->words);
        	word_list->words = NULL;
    	}
    	if (word_list && word_list->word_rects) {
        	free(word_list->word_rects);
        	word_list->word_rects = NULL;
    	}
    	word_list->count = 0;
}
