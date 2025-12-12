#include "localization.h"

// Helper function to check if a pixel is black
static int is_black_pixel(gdImagePtr img, int x, int y) {
    if (x < 0 || x >= gdImageSX(img) || y < 0 || y >= gdImageSY(img)) {
        return 0;
    }
    
    int color = gdImageGetPixel(img, x, y);
    
    // For palette images (like yours), index 0 is black
    if (!gdImageTrueColor(img)) {
        return (color == 0);  // Simple: index 0 = black
    }
    
    // For true color images (not your case)
    int r = gdImageRed(img, color);
    int g = gdImageGreen(img, color);
    int b = gdImageBlue(img, color);
    return (r < 128 && g < 128 && b < 128);
}

int* compute_horizontal_projection(gdImagePtr img) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    int* projection = (int*)calloc(height, sizeof(int));
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (is_black_pixel(img, x, y)) {
                projection[y]++;
            }
        }
    }
    return projection;
}

int* compute_vertical_projection(gdImagePtr img) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    int* projection = (int*)calloc(width, sizeof(int));
    
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            if (is_black_pixel(img, x, y)) {
                projection[x]++;
            }
        }
    }
    return projection;
}

int* compute_horizontal_projection_within(gdImagePtr img, BoundingBox region) {
    int* projection = (int*)calloc(region.height, sizeof(int));
    
    for (int y = 0; y < region.height; y++) {
        for (int x = 0; x < region.width; x++) {
            if (is_black_pixel(img, region.x + x, region.y + y)) {
                projection[y]++;
            }
        }
    }
    return projection;
}

int* compute_vertical_projection_within(gdImagePtr img, BoundingBox region) {
    int* projection = (int*)calloc(region.width, sizeof(int));
    
    for (int x = 0; x < region.width; x++) {
        for (int y = 0; y < region.height; y++) {
            if (is_black_pixel(img, region.x + x, region.y + y)) {
                projection[x]++;
            }
        }
    }
    return projection;
}

// Find peaks in projection (for detecting rows/columns)
int* find_peaks(int* projection, int length, int min_height, 
                int min_distance, int* num_peaks) {
    int* peaks = (int*)malloc(length * sizeof(int));
    int count = 0;
    
    for (int i = 1; i < length - 1; i++) {
        // A peak is a local maximum
        if (projection[i] > projection[i-1] && 
            projection[i] > projection[i+1] &&
            projection[i] > min_height) {
            
            // Check distance from previous peak
            if (count == 0 || i - peaks[count-1] >= min_distance) {
                peaks[count++] = i;
            }
        }
    }
    
    *num_peaks = count;
    return peaks;
}

// Find valleys in projection (for detecting spaces between rows/columns)
int* find_valleys(int* projection, int length, int max_height,
                  int min_distance, int* num_valleys) {
    int* valleys = (int*)malloc(length * sizeof(int));
    int count = 0;
    
    for (int i = 1; i < length - 1; i++) {
        // A valley is a local minimum
        if (projection[i] < projection[i-1] && 
            projection[i] < projection[i+1] &&
            projection[i] < max_height) {
            
            if (count == 0 || i - valleys[count-1] >= min_distance) {
                valleys[count++] = i;
            }
        }
    }
    
    *num_valleys = count;
    return valleys;
}