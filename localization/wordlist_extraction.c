// wordlist_extraction.c - Complete implementation for extracting letters from word list
#include "localization.h"
#include <string.h>

// Helper: Find word boundaries using horizontal projection
static int* find_word_rows(int* h_proj, int height, int* num_words) {
    int* word_rows = (int*)malloc(height * sizeof(int));
    int count = 0;
    int in_word = 0;
    int min_word_height = 10; // Minimum pixels for a word row
    int word_start = 0;
    
    for (int y = 0; y < height; y++) {
        if (h_proj[y] > 5) { // Threshold: at least 5 black pixels
            if (!in_word) {
                word_start = y;
                in_word = 1;
            }
        } else {
            if (in_word && (y - word_start) > min_word_height) {
                // Store middle of word row
                word_rows[count++] = (word_start + y) / 2;
            }
            in_word = 0;
        }
    }
    
    // Check last word
    if (in_word && (height - word_start) > min_word_height) {
        word_rows[count++] = (word_start + height) / 2;
    }
    
    *num_words = count;
    return word_rows;
}

// Helper: Find exact bounds of a word row
static void find_word_bounds(gdImagePtr img, BoundingBox region, int row_center, 
                            int* top, int* bottom, int* left, int* right) {
    int width = region.width;
    int height = region.height;
    
    // Find top boundary
    *top = row_center;
    for (int y = row_center; y >= 0; y--) {
        int black_count = 0;
        for (int x = 0; x < width; x++) {
            if (gdImageGetPixel(img, region.x + x, region.y + y) == 0) {
                black_count++;
            }
        }
        if (black_count < 3) {
            *top = y + 1;
            break;
        }
        if (y == 0) *top = 0;
    }
    
    // Find bottom boundary
    *bottom = row_center;
    for (int y = row_center; y < height; y++) {
        int black_count = 0;
        for (int x = 0; x < width; x++) {
            if (gdImageGetPixel(img, region.x + x, region.y + y) == 0) {
                black_count++;
            }
        }
        if (black_count < 3) {
            *bottom = y - 1;
            break;
        }
        if (y == height - 1) *bottom = height - 1;
    }
    
    // Find left and right boundaries
    *left = width;
    *right = 0;
    for (int y = *top; y <= *bottom; y++) {
        for (int x = 0; x < width; x++) {
            if (gdImageGetPixel(img, region.x + x, region.y + y) == 0) {
                if (x < *left) *left = x;
                if (x > *right) *right = x;
            }
        }
    }
}

// Helper: Find letter boundaries in a word row using vertical projection
static int* find_letter_columns(gdImagePtr img, BoundingBox word_region, int* num_letters) {
    int* v_proj = compute_vertical_projection_within(img, word_region);
    int width = word_region.width;
    
    // Find gaps (valleys) between letters
    int* letter_positions = (int*)malloc(width * sizeof(int));
    int count = 0;
    int in_letter = 0;
    int letter_start = 0;
    int min_letter_width = 3;
    
    for (int x = 0; x < width; x++) {
        if (v_proj[x] > 2) { // At least 2 black pixels in column
            if (!in_letter) {
                letter_start = x;
                in_letter = 1;
            }
        } else {
            if (in_letter && (x - letter_start) > min_letter_width) {
                // Store left and right boundaries
                letter_positions[count * 2] = letter_start;
                letter_positions[count * 2 + 1] = x - 1;
                count++;
            }
            in_letter = 0;
        }
    }
    
    // Check last letter
    if (in_letter && (width - letter_start) > min_letter_width) {
        letter_positions[count * 2] = letter_start;
        letter_positions[count * 2 + 1] = width - 1;
        count++;
    }
    
    free(v_proj);
    *num_letters = count;
    return letter_positions;
}

// Helper: Extract and center a single letter to 28x28
static gdImagePtr extract_and_center_letter(gdImagePtr img, BoundingBox letter_bbox) {
    int width = letter_bbox.width;
    int height = letter_bbox.height;
    
    // Find tight bounds of actual letter pixels
    int min_x = width, max_x = 0, min_y = height, max_y = 0;
    int pixel_count = 0;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (gdImageGetPixel(img, letter_bbox.x + x, letter_bbox.y + y) == 0) {
                pixel_count++;
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
            }
        }
    }
    
    // If too few pixels (noise), return white image
    if (pixel_count < 5) {
        gdImagePtr result = gdImageCreate(28, 28);
        int white = gdImageColorAllocate(result, 255, 255, 255);
        gdImageFilledRectangle(result, 0, 0, 27, 27, white);
        return result;
    }
    
    // Calculate bounding box with small padding
    int bbox_width = max_x - min_x + 1;
    int bbox_height = max_y - min_y + 1;
    int padding = 2;
    
    min_x = (min_x > padding) ? min_x - padding : 0;
    max_x = (max_x + padding < width) ? max_x + padding : width - 1;
    min_y = (min_y > padding) ? min_y - padding : 0;
    max_y = (max_y + padding < height) ? max_y + padding : height - 1;
    
    bbox_width = max_x - min_x + 1;
    bbox_height = max_y - min_y + 1;
    
    // Calculate scaling to fit in 28x28 while preserving aspect ratio
    float scale_x = 28.0f / bbox_width;
    float scale_y = 28.0f / bbox_height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    // Don't scale up too much
    if (scale > 1.5f) scale = 1.5f;
    
    int scaled_width = (int)(bbox_width * scale);
    int scaled_height = (int)(bbox_height * scale);
    
    // Create 28x28 image
    gdImagePtr result = gdImageCreate(28, 28);
    int white = gdImageColorAllocate(result, 255, 255, 255);
    int black = gdImageColorAllocate(result, 0, 0, 0);
    gdImageFilledRectangle(result, 0, 0, 27, 27, white);
    
    // Calculate centering offsets
    int offset_x = (28 - scaled_width) / 2;
    int offset_y = (28 - scaled_height) / 2;
    
    // Scale and copy
    for (int y = 0; y < scaled_height; y++) {
        for (int x = 0; x < scaled_width; x++) {
            int src_x = letter_bbox.x + min_x + (int)(x / scale);
            int src_y = letter_bbox.y + min_y + (int)(y / scale);
            
            if (gdImageGetPixel(img, src_x, src_y) == 0) {
                gdImageSetPixel(result, offset_x + x, offset_y + y, black);
            }
        }
    }
    
    return result;
}

// Main function: Extract all letters from word list
LetterData** extract_wordlist_letters(gdImagePtr img, BoundingBox wordlist, 
                                     int* num_words, int** letters_per_word) {
    printf("Extracting letters from word list (%dx%d)...\n", 
           wordlist.width, wordlist.height);
    
    // 1. Find word rows using horizontal projection
    int* h_proj = compute_horizontal_projection_within(img, wordlist);
    int* word_row_positions = find_word_rows(h_proj, wordlist.height, num_words);
    
    printf("Found %d words in list\n", *num_words);
    
    if (*num_words == 0) {
        free(h_proj);
        free(word_row_positions);
        *letters_per_word = NULL;
        return NULL;
    }
    
    // 2. Allocate arrays
    LetterData** all_letters = (LetterData**)malloc(*num_words * sizeof(LetterData*));
    *letters_per_word = (int*)malloc(*num_words * sizeof(int));
    
    // 3. Process each word
    for (int w = 0; w < *num_words; w++) {
        int row_center = word_row_positions[w];
        int top, bottom, left, right;
        
        // Find exact bounds of this word
        find_word_bounds(img, wordlist, row_center, &top, &bottom, &left, &right);
        
        BoundingBox word_region = {
            wordlist.x + left,
            wordlist.y + top,
            right - left + 1,
            bottom - top + 1
        };
        
        // Find individual letters in this word
        int num_letters_in_word;
        int* letter_positions = find_letter_columns(img, word_region, &num_letters_in_word);
        
        (*letters_per_word)[w] = num_letters_in_word;
        all_letters[w] = (LetterData*)malloc(num_letters_in_word * sizeof(LetterData));
        
        printf("  Word %d: %d letters\n", w, num_letters_in_word);
        
        // Extract each letter
        for (int l = 0; l < num_letters_in_word; l++) {
            int letter_left = letter_positions[l * 2];
            int letter_right = letter_positions[l * 2 + 1];
            
            BoundingBox letter_bbox = {
                word_region.x + letter_left,
                word_region.y,
                letter_right - letter_left + 1,
                word_region.height
            };
            
            // Extract and center to 28x28
            gdImagePtr letter_img = extract_and_center_letter(img, letter_bbox);
            
            // Store letter data
            all_letters[w][l].bbox = letter_bbox;
            all_letters[w][l].letter_img = letter_img;
            all_letters[w][l].predicted_char = '?';
            all_letters[w][l].row = w;
            all_letters[w][l].col = l;
        }
        
        free(letter_positions);
    }
    
    free(h_proj);
    free(word_row_positions);
    
    int total_letters = 0;
    for (int i = 0; i < *num_words; i++) {
        total_letters += (*letters_per_word)[i];
    }
    printf("Extracted %d total letters from word list\n", total_letters);
    
    return all_letters;
}

// Save all wordlist letters as JPG files
void save_wordlist_letters(LetterData** letters, int num_words, 
                          int* letters_per_word, const char* output_dir) {
    int letter_index = 0;
    
    for (int w = 0; w < num_words; w++) {
        for (int l = 0; l < letters_per_word[w]; l++) {
            if (letters[w][l].letter_img) {
                char filename[512];
                snprintf(filename, sizeof(filename), 
                        "%s/word_%02d_letter_%02d.jpg", output_dir, w, l);
                
                save_cell_as_jpg(letters[w][l].letter_img, filename, 28);
                letter_index++;
            }
        }
    }
    
    printf("Saved %d wordlist letters to %s\n", letter_index, output_dir);
}