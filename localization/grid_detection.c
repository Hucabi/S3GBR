#include "localization.h"

// Add forward declarations for helper functions
static void flood_fill_helper(gdImagePtr img, int x, int y, BoundingBox* bbox, int** visited, int width, int height);

BoundingBox find_grid_by_projection(gdImagePtr img) {
    BoundingBox grid = {0, 0, 0, 0};
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    
    // 1. Compute projections
    int* h_proj = compute_horizontal_projection(img);
    int* v_proj = compute_vertical_projection(img);
    
    // 2. Find grid boundaries using thresholding
    // For Level 1: Borders create continuous lines
    int border_threshold = width * 0.7; // Border covers at least 70% of width
    
    // Find top border
    for (int y = 0; y < height; y++) {
        if (h_proj[y] > border_threshold) {
            grid.y = y;
            break;
        }
    }
    
    // Find bottom border
    for (int y = height - 1; y >= 0; y--) {
        if (h_proj[y] > border_threshold) {
            grid.height = y - grid.y;
            break;
        }
    }
    
    // Find left border
    for (int x = 0; x < width; x++) {
        if (v_proj[x] > border_threshold) {
            grid.x = x;
            break;
        }
    }
    
    // Find right border
    for (int x = width - 1; x >= 0; x--) {
        if (v_proj[x] > border_threshold) {
            grid.width = x - grid.x;
            break;
        }
    }
    
    free(h_proj);
    free(v_proj);
    
    // If no borders found (Level 2/3), use alternative method
    if (grid.width == 0 || grid.height == 0) {
        return find_grid_by_text_clustering(img);
    }
    
    return grid;
}

BoundingBox find_grid_by_text_clustering(gdImagePtr img) {
    BoundingBox grid = {0, 0, 0, 0};
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    
    // 1. Find all connected components (letters)
    int** visited = (int**)malloc(height * sizeof(int*));
    for (int i = 0; i < height; i++) {
        visited[i] = (int*)calloc(width, sizeof(int));
    }
    
    // List of letter bounding boxes
    BoundingBox* letters = NULL;
    int num_letters = 0;
    
    // 2. Flood fill to find individual letters
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (!visited[y][x] && gdImageGetPixel(img, x, y) == 0) {
                BoundingBox bbox = {x, y, 0, 0};
                flood_fill(img, x, y, &bbox, visited);
                
                // Filter by size (remove noise, keep letters)
                if (bbox.width > 5 && bbox.height > 5 && 
                    bbox.width < 100 && bbox.height < 100) {
                    letters = realloc(letters, (num_letters + 1) * sizeof(BoundingBox));
                    letters[num_letters++] = bbox;
                }
            }
        }
    }
    
    // 3. Cluster letters into grid pattern
    if (num_letters > 0) {
        // Find min/max coordinates of letters
        int min_x = letters[0].x;
        int max_x = letters[0].x + letters[0].width;
        int min_y = letters[0].y;
        int max_y = letters[0].y + letters[0].height;
        
        for (int i = 1; i < num_letters; i++) {
            if (letters[i].x < min_x) min_x = letters[i].x;
            if (letters[i].x + letters[i].width > max_x) max_x = letters[i].x + letters[i].width;
            if (letters[i].y < min_y) min_y = letters[i].y;
            if (letters[i].y + letters[i].height > max_y) max_y = letters[i].y + letters[i].height;
        }
        
        grid.x = min_x;
        grid.y = min_y;
        grid.width = max_x - min_x;
        grid.height = max_y - min_y;
    }
    
    // Cleanup
    for (int i = 0; i < height; i++) free(visited[i]);
    free(visited);
    free(letters);
    
    return grid;
}

// Public wrapper function
void flood_fill(gdImagePtr img, int x, int y, BoundingBox* bbox, int** visited) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    flood_fill_helper(img, x, y, bbox, visited, width, height);
}

// Recursive helper (make it static to avoid linker issues)
static void flood_fill_helper(gdImagePtr img, int x, int y, BoundingBox* bbox, int** visited, int width, int height) {
    // Check bounds
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    
    // Check if visited or not a black pixel
    if (visited[y][x] || gdImageGetPixel(img, x, y) != 0) return;
    
    visited[y][x] = 1;
    
    // Update bounding box
    if (x < bbox->x) bbox->x = x;
    if (y < bbox->y) bbox->y = y;
    if (x - bbox->x + 1 > bbox->width) bbox->width = x - bbox->x + 1;
    if (y - bbox->y + 1 > bbox->height) bbox->height = y - bbox->y + 1;
    
    // Recursive 4-direction flood fill
    flood_fill_helper(img, x + 1, y, bbox, visited, width, height);
    flood_fill_helper(img, x - 1, y, bbox, visited, width, height);
    flood_fill_helper(img, x, y + 1, bbox, visited, width, height);
    flood_fill_helper(img, x, y - 1, bbox, visited, width, height);
}