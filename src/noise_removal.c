#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Remove isolated single pixels (salt-and-pepper noise)
void remove_single_pixel_noise(unsigned char* bw, int width, int height) {
    unsigned char* temp = (unsigned char*)malloc(width * height);
    if (!temp) return;
    memcpy(temp, bw, width * height);
    
    int noise_removed = 0;
    
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int idx = y * width + x;
            
            // Only check black pixels
            if (temp[idx] == 0) {
                int black_neighbors = 0;
                
                // Check 8 neighbors
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        
                        int nidx = (y + dy) * width + (x + dx);
                        if (temp[nidx] == 0) {
                            black_neighbors++;
                        }
                    }
                }
                
                // If a black pixel has 0 or 1 black neighbors, it's isolated noise
                if (black_neighbors <= 1) {
                    bw[idx] = 255; // Remove it (make white)
                    noise_removed++;
                }
            }
        }
    }
    
    free(temp);
    printf("Removed %d isolated single pixels\n", noise_removed);
}

// Remove tiny dots (2-4 pixels)
void remove_tiny_dots(unsigned char* bw, int width, int height) {
    unsigned char* visited = (unsigned char*)calloc(width * height, sizeof(unsigned char));
    if (!visited) return;
    
    int dots_removed = 0;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            
            if (bw[idx] == 0 && !visited[idx]) {
                // Simple flood fill to find connected black pixels
                int* stack_x = (int*)malloc(width * height * sizeof(int));
                int* stack_y = (int*)malloc(width * height * sizeof(int));
                int stack_ptr = 0;
                
                stack_x[stack_ptr] = x;
                stack_y[stack_ptr] = y;
                stack_ptr++;
                visited[idx] = 1;
                
                int component_size = 0;
                int min_x = x, max_x = x;
                int min_y = y, max_y = y;
                
                while (stack_ptr > 0) {
                    stack_ptr--;
                    int cx = stack_x[stack_ptr];
                    int cy = stack_y[stack_ptr];
                    
                    component_size++;
                    if (cx < min_x) min_x = cx;
                    if (cx > max_x) max_x = cx;
                    if (cy < min_y) min_y = cy;
                    if (cy > max_y) max_y = cy;
                    
                    // Check 4-connected neighbors
                    int dx[] = {-1, 1, 0, 0};
                    int dy[] = {0, 0, -1, 1};
                    
                    for (int i = 0; i < 4; i++) {
                        int nx = cx + dx[i];
                        int ny = cy + dy[i];
                        
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int nidx = ny * width + nx;
                            if (bw[nidx] == 0 && !visited[nidx]) {
                                visited[nidx] = 1;
                                stack_x[stack_ptr] = nx;
                                stack_y[stack_ptr] = ny;
                                stack_ptr++;
                            }
                        }
                    }
                }
                
                free(stack_x);
                free(stack_y);
                
                // SAFE CRITERIA: Only remove if it's VERY small and compact
                // 1-4 pixels AND in a small bounding box
                int bbox_width = max_x - min_x + 1;
                int bbox_height = max_y - min_y + 1;
                
                if (component_size <= 4 && 
                    bbox_width <= 3 && 
                    bbox_height <= 3) {
                    // Remove this tiny dot
                    for (int cy = min_y; cy <= max_y; cy++) {
                        for (int cx = min_x; cx <= max_x; cx++) {
                            int cidx = cy * width + cx;
                            if (bw[cidx] == 0) {
                                bw[cidx] = 255;
                                dots_removed++;
                            }
                        }
                    }
                }
            }
        }
    }
    
    free(visited);
    printf("Removed %d tiny dots (2-4 pixels)\n", dots_removed);
}

// Remove thin line artifacts (scanner lines)
void remove_thin_lines(unsigned char* bw, int width, int height) {
    // Remove single-pixel vertical lines longer than 10 pixels
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            int idx = y * width + x;
            
            if (bw[idx] == 0) {
                // Check vertical line
                int line_length = 1;
                while (y + line_length < height && 
                       bw[(y + line_length) * width + x] == 0) {
                    line_length++;
                }
                
                // Check if it's isolated (no black pixels on left or right)
                if (line_length > 10) {
                    int isolated = 1;
                    for (int ly = y; ly < y + line_length && isolated; ly++) {
                        // Check left and right
                        if (x > 0 && bw[ly * width + (x - 1)] == 0) isolated = 0;
                        if (x < width - 1 && bw[ly * width + (x + 1)] == 0) isolated = 0;
                    }
                    
                    if (isolated) {
                        // Remove this vertical line
                        for (int ly = y; ly < y + line_length; ly++) {
                            bw[ly * width + x] = 255;
                        }
                        y += line_length - 1;
                    }
                }
            }
        }
    }
    
    // Remove single-pixel horizontal lines longer than 10 pixels
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            
            if (bw[idx] == 0) {
                // Check horizontal line
                int line_length = 1;
                while (x + line_length < width && 
                       bw[y * width + (x + line_length)] == 0) {
                    line_length++;
                }
                
                // Check if it's isolated (no black pixels above or below)
                if (line_length > 10) {
                    int isolated = 1;
                    for (int lx = x; lx < x + line_length && isolated; lx++) {
                        // Check above and below
                        if (y > 0 && bw[(y - 1) * width + lx] == 0) isolated = 0;
                        if (y < height - 1 && bw[(y + 1) * width + lx] == 0) isolated = 0;
                    }
                    
                    if (isolated) {
                        // Remove this horizontal line
                        for (int lx = x; lx < x + line_length; lx++) {
                            bw[y * width + lx] = 255;
                        }
                        x += line_length - 1;
                    }
                }
            }
        }
    }
    
    printf("Removed thin line artifacts\n");
}

// Main preprocessing - just removes obvious noise
void preprocess_ocr(unsigned char* img, unsigned char* bw, 
                    int width, int height, int channels) {
    
    // Function prototypes
    void rgb_to_gray(unsigned char* img, unsigned char* gray, int width, int height, int channels);
    void binarize(unsigned char* gray, unsigned char* bw_logic, int width, int height);
    
    printf("\n=== SIMPLE NOISE REMOVAL ===\n");
    printf("Only removing obvious noise, preserving all text\n");
    
    // Step 1: Convert to grayscale
    unsigned char* gray = (unsigned char*)malloc(width * height);
    if (!gray) return;
    
    rgb_to_gray(img, gray, width, height, channels);
    
    // Step 2: Binarize
    unsigned char* bw_logic = (unsigned char*)malloc(width * height);
    if (!bw_logic) {
        free(gray);
        return;
    }
    
    binarize(gray, bw_logic, width, height);
    
    // Convert 0/1 to 0/255 for visualization
    for (int i = 0; i < width * height; i++) {
        bw[i] = bw_logic[i] ? 0 : 255; // 0=black, 255=white
    }
    
    // Save original binarized image
    stbi_write_png("data/debug/00_original_binary.png", width, height, 1, bw, width);
    
    // Step 3: Remove isolated single pixels (obvious noise)
    printf("1. Removing isolated single pixels...\n");
    remove_single_pixel_noise(bw, width, height);
    stbi_write_png("data/debug/01_after_single_pixels.png", width, height, 1, bw, width);
    
    // Step 4: Remove tiny dots (2-4 pixels)
    printf("2. Removing tiny dots...\n");
    remove_tiny_dots(bw, width, height);
    stbi_write_png("data/debug/02_after_tiny_dots.png", width, height, 1, bw, width);
    
    // Step 5: Remove thin line artifacts (optional)
    printf("3. Removing thin line artifacts...\n");
    remove_thin_lines(bw, width, height);
    stbi_write_png("data/debug/03_final_cleaned.png", width, height, 1, bw, width);
    
    // Clean up
    free(gray);
    free(bw_logic);
    
    printf("Noise removal complete! Text should be fully preserved.\n");
    printf("Check debug images in data/debug/ to see what was removed\n");
}
