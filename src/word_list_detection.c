#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "word_list_detection.h"

typedef struct {
    int count;
    char **words;           // Recognized words
    Rectangle *word_rects;  // Position of each word
    Rectangle list_region;  // Global region of the list
    TextOrientation orientation; // Horizontal or vertical
    int words_per_column;   // For mutiple-columns lists
} WordList;

typedef enum {
    ORIENTATION_HORIZONTAL,
    ORIENTATION_VERTICAL,
    ORIENTATION_MULTICOLUMN
} TextOrientation;

WordList find_word_list(BinaryImage *img, GridCells grid)
{
	WordList word_list = {0, NULL, NULL, {0,0,0,0}, ORIENTATION_HORIZONTAL, 1};
    
    	Rectangle grid_region = find_grid_bounding_box(grid);
    
    	Rectangle candidates[4] = {
        	{0, 0, grid_region.x, img->height}, // Left
        	{grid_region.x + grid_region.w, 0, 
         	img->width - (grid_region.x + grid_region.w), img->height}, // Right
        	{0, 0, img->width, grid_region.y}, // Up
        	{0, grid_region.y + grid_region.h, 
         	img->width, img->height - (grid_region.y + grid_region.h)} // Down
    	};
    
    	double best_score = 0;
    	Rectangle best_region = {0,0,0,0};
    	TextOrientation best_orientation = ORIENTATION_HORIZONTAL;
    
    	for (int i = 0; i < 4; i++) {
        	Rectangle region = candidates[i];
        	if (region.w < 50 || region.h < 50) continue; // Too small
        
        	double score;
        	TextOrientation orientation;
        	analyze_text_region(img, region, &score, &orientation);
        
        	if (score > best_score) {
            		best_score = score;
            		best_region = region;
            		best_orientation = orientation;
        	}
    	}
    
    	if (best_score > 0.1) { // Threshold
        	word_list.list_region = best_region;
        	word_list.orientation = best_orientation;
        	printf("List region found: x=%d, y=%d, w=%d, h=%d, orientation=%d, score=%.2f\n",
               		best_region.x, best_region.y, best_region.w, best_region.h,
               		best_orientation, best_score);
        
        	// Extract words from the region
        	word_list = extract_words_from_region(img, best_region, best_orientation);
    	}
	else
	{
        	printf("Warning: No word list region found with sufficient confidence\n");
    	}
    
    	return word_list;
}

void analyze_text_region(BinaryImage *img, Rectangle region,
	double *score, TextOrientation *orientation)
{
    	BinaryImage *sub_img = extract_region(img, region);

    	// Compute projections
    	int *h_proj = calloc(sub_img->height, sizeof(int));
    	int *v_proj = calloc(sub_img->width, sizeof(int));

    	for (int y = 0; y < sub_img->height; y++) {
        	for (int x = 0; x < sub_img->width; x++) {
            		if (*binary_pixel_at(sub_img, x, y) == 0) {
                		h_proj[y]++;
                		v_proj[x]++;
            		}
        	}
    	}

    	// Analyse distribution to determine orientation
    	double h_regularity = calculate_regularity(h_proj, sub_img->height);
    	double v_regularity = calculate_regularity(v_proj, sub_img->width);

    	// Compute text density
    	int total_black = 0;
    	for (int i = 0; i < sub_img->width * sub_img->height; i++) {
        	if (sub_img->data[i] == 0) total_black++;
    	}
    	double density = (double)total_black / (sub_img->width * sub_img->height);

    	// Determine orientation
    	if (h_regularity > v_regularity * 1.5) {
        	*orientation = ORIENTATION_VERTICAL; // Words in column
    	}
	else if (v_regularity > h_regularity * 1.5) {
        	*orientation = ORIENTATION_HORIZONTAL; // Words in row
    	}
	else {
        	*orientation = ORIENTATION_MULTICOLUMN; // Mix 
    	}

    	// Score based on regularity and density
    	*score = (h_regularity + v_regularity) * density;

    	free(h_proj);
    	free(v_proj);
    	binary_image_free(sub_img);
}

double calculate_regularity(int *projection, int length)
{
    	if (length <= 1) return 0;

    	// Compute number of peaks (significant ones)
    	int peak_count;
    	int *peaks = find_peaks(projection, length, &peak_count, 0.1);

    	// Compute regularity of the spaces
    	if (peak_count < 2) {
        	free(peaks);
        	return 0;
    	}

    	double total_gap = 0;
    	double avg_gap = 0;
    	for (int i = 1; i < peak_count; i++) {
        	total_gap += peaks[i] - peaks[i-1];
    	}
    	avg_gap = total_gap / (peak_count - 1);

    	// Compute variance of the spaces
    	double variance = 0;
    	for (int i = 1; i < peak_count; i++) {
        	double diff = (peaks[i] - peaks[i-1]) - avg_gap;
        	variance += diff * diff;
    	}
    	variance /= (peak_count - 1);
    	double std_dev = sqrt(variance);

    	// Régularity = opposite of normalized variance
    	double regularity = avg_gap / (std_dev + 1);

    	free(peaks);
    	return regularity;
}

WordList extract_words_from_region(BinaryImage *img, Rectangle region,
	TextOrientation orientation)
{
    	WordList word_list = {0, NULL, NULL, region, orientation, 1};
    	BinaryImage *region_img = extract_region(img, region);

    	Lines lines;
    	switch (orientation) {
        	case ORIENTATION_VERTICAL:
            		lines = extract_vertical_lines(region_img);
            		break;
        	case ORIENTATION_HORIZONTAL:
            		lines = extract_horizontal_lines(region_img);
            		break;
        	case ORIENTATION_MULTICOLUMN:
            		lines = extract_multicolumn_lines(region_img);
            		break;
    	}

    	printf("Extracted %d lines from word list\n", lines.count);

    	word_list.count = lines.count;
    	word_list.words = calloc(lines.count, sizeof(char*));
    	word_list.word_rects = malloc(lines.count * sizeof(Rectangle));

    	// Extract letter of each row
    	for (int i = 0; i < lines.count; i++) {
        	extract_letters_from_word_line(region_img, lines.rects[i], i, &word_list);
    	}

    	binary_image_free(region_img);
    	lines_free(&lines);
    	return word_list;
}

Lines extract_horizontal_lines(BinaryImage *img)
{
    	Lines lines = {0, NULL};

    	// Horizontal projection
    	int *h_proj = calloc(img->height, sizeof(int));
    	for (int y = 0; y < img->height; y++) {
        	for (int x = 0; x < img->width; x++) {
            	if (*binary_pixel_at(img, x, y) == 0) h_proj[y]++;
        	}
    	}

    	// Detection of peaks (text row)
    	int peak_count;
    	int *peaks = find_peaks(h_proj, img->height, &peak_count, 0.2);

    	// Estimate raw height 
    	int line_height = estimate_line_height(peaks, peak_count, img->height);

    	lines.count = peak_count;
    	lines.rects = malloc(peak_count * sizeof(Rectangle));

    	for (int i = 0; i < peak_count; i++) {
        	int center_y = peaks[i];
        	lines.rects[i] = (Rectangle){
            	0,
            	center_y - line_height/2,
            	img->width,
            	line_height
        	};
        	// Adjust limits
        	if (lines.rects[i].y < 0) lines.rects[i].y = 0;
        	if (lines.rects[i].y + lines.rects[i].h > img->height) {
           	 	lines.rects[i].h = img->height - lines.rects[i].y;
        	}
    	}

    	free(h_proj);
    	free(peaks);
    	return lines;
}

Lines extract_vertical_lines(BinaryImage *img)
{
    	Lines lines = {0, NULL};

    	// Vertical projection
    	int *v_proj = calloc(img->width, sizeof(int));
    	for (int x = 0; x < img->width; x++) {
        	for (int y = 0; y < img->height; y++) {
            		if (*binary_pixel_at(img, x, y) == 0) v_proj[x]++;
        		}
    		}

    	// Detection of peaks (text columns)
    	int peak_count;
    	int *peaks = find_peaks(v_proj, img->width, &peak_count, 0.2);

    	// Estimate column width
    	int col_width = estimate_column_width(peaks, peak_count, img->width);

    	lines.count = peak_count;
    	lines.rects = malloc(peak_count * sizeof(Rectangle));

    	for (int i = 0; i < peak_count; i++) {
        	int center_x = peaks[i];
        	lines.rects[i] = (Rectangle){
            	center_x - col_width/2,
            	0,
            	col_width,
            	img->height
        	};
        	// Adjust limits
        	if (lines.rects[i].x < 0) lines.rects[i].x = 0;
        	if (lines.rects[i].x + lines.rects[i].w > img->width) {
            		lines.rects[i].w = img->width - lines.rects[i].x;
        	}
    	}

    	free(v_proj);
    	free(peaks);
    	return lines;
}

Lines extract_multicolumn_lines(BinaryImage *img)
{
    	Lines lines = {0, NULL};

    	// Text block detection with connected components
    	TextBlock *blocks = detect_text_blocks(img, &lines.count);

    	lines.rects = malloc(lines.count * sizeof(Rectangle));
    	for (int i = 0; i < lines.count; i++) {
        	lines.rects[i] = blocks[i].bounding_box;
   	}

    	free(blocks);
    	return lines;
}

typedef struct {
    	Rectangle bounding_box;
    	int letter_count;
} TextBlock;

TextBlock* detect_text_blocks(BinaryImage *img, int *block_count)
{
    	*block_count = 0;
    	TextBlock *blocks = malloc(100 * sizeof(TextBlock)); // Set max length

    	// Horizontal projection to find rows
    	int *h_proj = calloc(img->height, sizeof(int));
    	for (int y = 0; y < img->height; y++) {
        	for (int x = 0; x < img->width; x++) {
            		if (*binary_pixel_at(img, x, y) == 0) h_proj[y]++;
        		}
    		}

    		int line_count;
    		int *line_peaks = find_peaks(h_proj, img->height, &line_count, 0.15);

    		// foreach row, find the words using vertical projection
    		for (int i = 0; i < line_count; i++) {
        		int line_y = line_peaks[i];
        		int line_h = 20; // Estimation

        	// Vertical projection of the row
        	int *line_v_proj = calloc(img->width, sizeof(int));
        	for (int x = 0; x < img->width; x++) {
        		for (int y = line_y - line_h/2; y < line_y + line_h/2; y++) {
                		if (y >= 0 && y < img->height && *binary_pixel_at(img, x, y) == 0) {
                    	line_v_proj[x]++;
                		}
            		}
        	}

        	// Find words (letter groups)
        	int word_count;
        	Rectangle *word_rects = find_words_in_line(line_v_proj, img->width,
			line_y, line_h, &word_count);

        	// Add words to block
        	for (int w = 0; w < word_count; w++) {
            		if (*block_count < 100) {
                		blocks[*block_count].bounding_box = word_rects[w];
                		blocks[*block_count].letter_count = 0; // need to determine
                		(*block_count)++;
            		}
		}

        	free(line_v_proj);
        	free(word_rects);
    	}

    	free(h_proj);
    	free(line_peaks);
    	return blocks;
}

void extract_letters_from_word_line(BinaryImage *region_img, Rectangle word_rect,
		int word_index, WordList *word_list)
{
    	// Extract image of the word
    	BinaryImage *word_img = extract_region(region_img, word_rect);

    	// Sauvegarder la position dans l'image originale
    	word_list->word_rects[word_index] = (Rectangle){
        	word_list->list_region.x + word_rect.x,
        	word_list->list_region.y + word_rect.y,
        	word_rect.w,
        	word_rect.h
    	};

    	// Extract individual letters
    	Letters letters = extract_letters_from_line(word_img);
    	printf("Word %d: %d letters detected\n", word_index, letters.count);

    	// Save each letters for OCR
    	for (int j = 0; j < letters.count; j++) {
        	BinaryImage *letter_img = extract_cell_content(word_img, letters.rects[j]);
        	if (letter_img) {
            		char filename[256];
            		snprintf(filename, sizeof(filename),
                    		"word_%d_letter_%d.png", word_index, j);
            		save_binary_image(letter_img, filename);
            		binary_image_free(letter_img);
        	}
    	}

    	letters_free(&letters);
    	binary_image_free(word_img);
}

Rectangle find_grid_bounding_box(GridCells grid)
{
    	if (grid.count == 0) return (Rectangle){0,0,0,0};

    	int min_x = grid.rects[0].x;
    	int min_y = grid.rects[0].y;
    	int max_x = grid.rects[0].x + grid.rects[0].w;
    	int max_y = grid.rects[0].y + grid.rects[0].h;

    	for (int i = 1; i < grid.count; i++) {
        	if (grid.rects[i].x < min_x) min_x = grid.rects[i].x;
        	if (grid.rects[i].y < min_y) min_y = grid.rects[i].y;
        	if (grid.rects[i].x + grid.rects[i].w > max_x)
            		max_x = grid.rects[i].x + grid.rects[i].w;
        	if (grid.rects[i].y + grid.rects[i].h > max_y)
			max_y = grid.rects[i].y + grid.rects[i].h;
    	}

    	return (Rectangle){min_x, min_y, max_x - min_x, max_y - min_y};
}

BinaryImage* extract_region(BinaryImage *img, Rectangle region)
{
    	BinaryImage *sub_img = binary_image_create(region.w, region.h);

    	for (int y = 0; y < region.h; y++) {
        	for (int x = 0; x < region.w; x++) {
            		int orig_x = region.x + x;
            		int orig_y = region.y + y;

            		if (orig_x < img->width && orig_y < img->height) {
                		sub_img->data[y * region.w + x] =
                    			*binary_pixel_at(img, orig_x, orig_y);
            		}
			else {
                		sub_img->data[y * region.w + x] = 255; // White
            		}
       	 	}
    	}

    	return sub_img;
}
