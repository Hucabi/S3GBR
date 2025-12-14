#include "localization.h"
#include <string.h>

// --- Text Check ---
static int region_has_wordlist(gdImagePtr img, BoundingBox region) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    
    if (region.x < 0) region.x = 0;
    if (region.y < 0) region.y = 0;
    if (region.x + region.width > width) region.width = width - region.x;
    if (region.y + region.height > height) region.height = height - region.y;

    if (region.width < 10 || region.height < 10) return 0;

    long black_pixels = 0;
    long total_pixels = (long)region.width * region.height;
    
    for (int y = region.y; y < region.y + region.height; y += 2) {
        for (int x = region.x; x < region.x + region.width; x += 2) {
            if (gdImageGetPixel(img, x, y) == 0) black_pixels++;
        }
    }
    black_pixels *= 4; 

    float density = (float)black_pixels / total_pixels;
    if (density < 0.005) return 0;

    return 1;
}

// --- Corrected Shrink Wrap (Full Scan) ---
// Scans entire region for bounds, ignoring internal gaps.
// This FIXES the Level 2 issue where the word list was cut in half.
static BoundingBox shrink_wrap_content(gdImagePtr img, BoundingBox region) {
    BoundingBox final = region;
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    
    int min_x = width, max_x = 0;
    int min_y = height, max_y = 0;
    int found_pixel = 0;

    // Scan EVERY pixel in the region to find extreme bounds
    for (int y = region.y; y < region.y + region.height; y++) {
        for (int x = region.x; x < region.x + region.width; x++) {
            if (x < width && y < height && gdImageGetPixel(img, x, y) == 0) {
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
                found_pixel = 1;
            }
        }
    }
    
    if (!found_pixel) return (BoundingBox){0,0,0,0};

    final.x = min_x;
    final.y = min_y;
    final.width = max_x - min_x + 1;
    final.height = max_y - min_y + 1;
    
    return final;
}

BoundingBox find_wordlist_region(gdImagePtr img, BoundingBox grid) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    
    printf("Searching for wordlist around grid...\n");
    
    BoundingBox regions[4];
    regions[0] = (BoundingBox){0, 0, grid.x, height}; // Left
    regions[1] = (BoundingBox){grid.x + grid.width, 0, width - (grid.x + grid.width), height}; // Right
    regions[2] = (BoundingBox){0, grid.y + grid.height, width, height - (grid.y + grid.height)}; // Bottom
    regions[3] = (BoundingBox){0, 0, width, grid.y}; // Top
    
    const char* names[] = {"LEFT", "RIGHT", "BOTTOM", "TOP"};
    // Bias against Top (fixes Level 3 Header selection)
    float weights[] = {1.0f, 1.2f, 1.2f, 0.05f}; 
    
    BoundingBox best_region = {0,0,0,0};
    float best_score = 0;
    
    for (int i = 0; i < 4; i++) {
        if(regions[i].width < 20 || regions[i].height < 20) continue;
        
        if (region_has_wordlist(img, regions[i])) {
            BoundingBox tight = shrink_wrap_content(img, regions[i]);
            
            // Re-check valid dimensions
            if (tight.width > 0 && tight.height > 0) {
                int area = tight.width * tight.height;
                float weighted_score = area * weights[i];
                
                printf("  %s region: Found. Area=%d, Score=%.0f\n", names[i], area, weighted_score);
                
                if (weighted_score > best_score) {
                    best_score = weighted_score;
                    best_region = tight;
                }
            }
        }
    }
    
    // Padding
    int pad = 5;
    best_region.x = (best_region.x > pad) ? best_region.x - pad : 0;
    best_region.y = (best_region.y > pad) ? best_region.y - pad : 0;
    best_region.width += pad * 2;
    best_region.height += pad * 2;
    
    if (best_region.x + best_region.width > width) best_region.width = width - best_region.x;
    if (best_region.y + best_region.height > height) best_region.height = height - best_region.y;
    
    return best_region;
}