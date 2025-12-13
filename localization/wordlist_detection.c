// wordlist_detection.c - IMPROVED VERSION with better detection
#include "localization.h"
#include <string.h>

// Helper: Check if a region contains text
static int region_has_text(gdImagePtr img, BoundingBox region, int threshold) {
    int black_pixels = 0;
    int total_pixels = 0;
    
    // Sample the region
    int step_x = (region.width > 10) ? region.width / 10 : 1;
    int step_y = (region.height > 10) ? region.height / 10 : 1;
    
    for (int y = region.y; y < region.y + region.height; y += step_y) {
        for (int x = region.x; x < region.x + region.width; x += step_x) {
            if (x >= 0 && x < gdImageSX(img) && y >= 0 && y < gdImageSY(img)) {
                total_pixels++;
                int pixel = gdImageGetPixel(img, x, y);
                if (pixel == 0) {
                    black_pixels++;
                }
            }
        }
    }
    
    if (total_pixels == 0) return 0;
    
    int percentage = (black_pixels * 100) / total_pixels;
    return percentage > threshold;
}

// Helper: Find exact text boundaries in a region
static BoundingBox find_text_boundaries(gdImagePtr img, BoundingBox region) {
    BoundingBox text = region;
    int img_width = gdImageSX(img);
    int img_height = gdImageSY(img);
    
    // Find top boundary
    for (int y = region.y; y < region.y + region.height && y < img_height; y++) {
        int row_black = 0;
        for (int x = region.x; x < region.x + region.width && x < img_width; x++) {
            if (gdImageGetPixel(img, x, y) == 0) {
                row_black++;
            }
        }
        if (row_black > 3) {
            text.y = y;
            break;
        }
    }
    
    // Find bottom boundary
    for (int y = region.y + region.height - 1; y >= region.y && y >= 0; y--) {
        int row_black = 0;
        for (int x = region.x; x < region.x + region.width && x < img_width; x++) {
            if (gdImageGetPixel(img, x, y) == 0) {
                row_black++;
            }
        }
        if (row_black > 3) {
            text.height = y - text.y + 1;
            break;
        }
    }
    
    // Find left boundary
    for (int x = region.x; x < region.x + region.width && x < img_width; x++) {
        int col_black = 0;
        for (int y = text.y; y < text.y + text.height && y < img_height; y++) {
            if (gdImageGetPixel(img, x, y) == 0) {
                col_black++;
            }
        }
        if (col_black > 3) {
            text.x = x;
            break;
        }
    }
    
    // Find right boundary
    for (int x = region.x + region.width - 1; x >= region.x && x >= 0; x--) {
        int col_black = 0;
        for (int y = text.y; y < text.y + text.height && y < img_height; y++) {
            if (gdImageGetPixel(img, x, y) == 0) {
                col_black++;
            }
        }
        if (col_black > 3) {
            text.width = x - text.x + 1;
            break;
        }
    }
    
    return text;
}

BoundingBox find_wordlist_region(gdImagePtr img, BoundingBox grid) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    
    printf("Searching for wordlist around grid...\n");
    printf("  Image: %dx%d, Grid: (%d,%d) %dx%d\n", 
           width, height, grid.x, grid.y, grid.width, grid.height);
    
    // Define regions around the grid
    BoundingBox regions[4];
    int margin = 10; // Small margin to avoid grid borders
    
    // Left region - MOST COMMON for wordlists
    regions[0].x = 0;
    regions[0].y = (grid.y > margin) ? grid.y - margin : 0;
    regions[0].width = (grid.x > margin) ? grid.x - margin : 0;
    regions[0].height = grid.height + 2 * margin;
    if (regions[0].height + regions[0].y > height) {
        regions[0].height = height - regions[0].y;
    }
    
    // Right region
    regions[1].x = grid.x + grid.width + margin;
    regions[1].y = (grid.y > margin) ? grid.y - margin : 0;
    regions[1].width = (regions[1].x < width) ? width - regions[1].x : 0;
    regions[1].height = grid.height + 2 * margin;
    if (regions[1].height + regions[1].y > height) {
        regions[1].height = height - regions[1].y;
    }
    
    // Top region
    regions[2].x = (grid.x > margin) ? grid.x - margin : 0;
    regions[2].y = 0;
    regions[2].width = grid.width + 2 * margin;
    if (regions[2].width + regions[2].x > width) {
        regions[2].width = width - regions[2].x;
    }
    regions[2].height = (grid.y > margin) ? grid.y - margin : 0;
    
    // Bottom region
    regions[3].x = (grid.x > margin) ? grid.x - margin : 0;
    regions[3].y = grid.y + grid.height + margin;
    regions[3].width = grid.width + 2 * margin;
    if (regions[3].width + regions[3].x > width) {
        regions[3].width = width - regions[3].x;
    }
    regions[3].height = (regions[3].y < height) ? height - regions[3].y : 0;
    
    const char* region_names[] = {"LEFT", "RIGHT", "TOP", "BOTTOM"};
    
    // Check each region for text
    BoundingBox best_region = {0, 0, 0, 0};
    int best_text_score = 0;
    int best_idx = -1;
    
    for (int i = 0; i < 4; i++) {
        printf("  Checking %s region: (%d,%d) %dx%d... ", 
               region_names[i], regions[i].x, regions[i].y, 
               regions[i].width, regions[i].height);
        
        if (regions[i].width > 20 && regions[i].height > 20) {
            if (region_has_text(img, regions[i], 3)) { // Lower threshold to 3%
                printf("TEXT FOUND! ");
                
                // Find exact boundaries
                BoundingBox text_region = find_text_boundaries(img, regions[i]);
                
                // Score based on size and aspect ratio
                int score = text_region.width * text_region.height;
                
                // Prefer regions that are distinctly vertical (wordlists are usually vertical)
                float aspect = (float)text_region.width / text_region.height;
                if (aspect < 0.5) { // Tall and narrow (vertical list)
                    score *= 3;
                } else if (aspect > 2.0) { // Wide and short (horizontal list)
                    score *= 2;
                }
                
                printf("Score=%d (aspect=%.2f)\n", score, aspect);
                
                if (score > best_text_score) {
                    best_text_score = score;
                    best_region = text_region;
                    best_idx = i;
                }
            } else {
                printf("no text\n");
            }
        } else {
            printf("too small\n");
        }
    }
    
    if (best_idx >= 0) {
        printf("  Best match: %s region with score %d\n", 
               region_names[best_idx], best_text_score);
    }
    
    return best_region;
}