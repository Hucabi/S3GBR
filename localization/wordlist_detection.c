// wordlist_detection.c - UNIVERSAL VERSION
#include "localization.h"
#include <string.h>

// Helper: Check if a region contains text
static int region_has_text(gdImagePtr img, BoundingBox region, int threshold) {
    int black_pixels = 0;
    
    // Sample some points in the region
    int step_x = region.width / 20;
    int step_y = region.height / 20;
    
    if (step_x < 1) step_x = 1;
    if (step_y < 1) step_y = 1;
    
    for (int y = region.y; y < region.y + region.height; y += step_y) {
        for (int x = region.x; x < region.x + region.width; x += step_x) {
            int pixel = gdImageGetPixel(img, x, y);
            // Check if pixel is black (0) or dark
            if (pixel == 0) {
                black_pixels++;
            }
        }
    }
    
    int total_samples = (region.width / step_x) * (region.height / step_y);
    return (black_pixels * 100 / total_samples) > threshold;
}

// Helper: Find exact text boundaries in a region
static BoundingBox find_text_boundaries(gdImagePtr img, BoundingBox region) {
    BoundingBox text = region;
    
    // Find top
    for (int y = region.y; y < region.y + region.height; y++) {
        int row_black = 0;
        for (int x = region.x; x < region.x + region.width; x++) {
            if (gdImageGetPixel(img, x, y) == 0) {
                row_black++;
            }
        }
        if (row_black > 5) {
            text.y = y;
            break;
        }
    }
    
    // Find bottom
    for (int y = region.y + region.height - 1; y >= region.y; y--) {
        int row_black = 0;
        for (int x = region.x; x < region.x + region.width; x++) {
            if (gdImageGetPixel(img, x, y) == 0) {
                row_black++;
            }
        }
        if (row_black > 5) {
            text.height = y - text.y + 1;
            break;
        }
    }
    
    // Find left
    for (int x = region.x; x < region.x + region.width; x++) {
        int col_black = 0;
        for (int y = text.y; y < text.y + text.height; y++) {
            if (gdImageGetPixel(img, x, y) == 0) {
                col_black++;
            }
        }
        if (col_black > 5) {
            text.x = x;
            break;
        }
    }
    
    // Find right
    for (int x = region.x + region.width - 1; x >= region.x; x--) {
        int col_black = 0;
        for (int y = text.y; y < text.y + text.height; y++) {
            if (gdImageGetPixel(img, x, y) == 0) {
                col_black++;
            }
        }
        if (col_black > 5) {
            text.width = x - text.x + 1;
            break;
        }
    }
    
    return text;
}

BoundingBox find_wordlist_region(gdImagePtr img, BoundingBox grid) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    
    // Define regions around the grid (with margins)
    BoundingBox regions[4];
    int margin = 50; // pixels margin around grid
    
    // Left region
    regions[0].x = 0;
    regions[0].y = grid.y - margin;
    regions[0].width = grid.x - margin;
    regions[0].height = grid.height + 2 * margin;
    if (regions[0].y < 0) regions[0].y = 0;
    if (regions[0].width < 0) regions[0].width = 0;
    if (regions[0].height > height) regions[0].height = height;
    
    // Right region
    regions[1].x = grid.x + grid.width + margin;
    regions[1].y = grid.y - margin;
    regions[1].width = width - regions[1].x;
    regions[1].height = grid.height + 2 * margin;
    if (regions[1].y < 0) regions[1].y = 0;
    if (regions[1].width < 0) regions[1].width = 0;
    if (regions[1].height > height) regions[1].height = height;
    
    // Top region
    regions[2].x = grid.x - margin;
    regions[2].y = 0;
    regions[2].width = grid.width + 2 * margin;
    regions[2].height = grid.y - margin;
    if (regions[2].x < 0) regions[2].x = 0;
    if (regions[2].width > width) regions[2].width = width;
    if (regions[2].height < 0) regions[2].height = 0;
    
    // Bottom region
    regions[3].x = grid.x - margin;
    regions[3].y = grid.y + grid.height + margin;
    regions[3].width = grid.width + 2 * margin;
    regions[3].height = height - regions[3].y;
    if (regions[3].x < 0) regions[3].x = 0;
    if (regions[3].width > width) regions[3].width = width;
    if (regions[3].height < 0) regions[3].height = 0;
    
    // Check each region for text
    BoundingBox best_region = {0, 0, 0, 0};
    int best_text_score = 0;
    
    for (int i = 0; i < 4; i++) {
        if (regions[i].width > 20 && regions[i].height > 20) {
            if (region_has_text(img, regions[i], 5)) { // 5% black pixels threshold
                // This region has text, find exact boundaries
                BoundingBox text_region = find_text_boundaries(img, regions[i]);
                
                // Score based on size and aspect ratio (word lists are usually tall or wide, not square)
                int score = text_region.width * text_region.height;
                
                // Prefer regions that are distinctly horizontal or vertical
                float aspect = (float)text_region.width / text_region.height;
                if (aspect > 3.0 || aspect < 0.33) { // Very horizontal or vertical
                    score *= 2;
                }
                
                if (score > best_text_score) {
                    best_text_score = score;
                    best_region = text_region;
                }
            }
        }
    }
    
    return best_region;
}