#include "localization.h"
#include <math.h>

// =========================================================
// STRATEGY A: LINE DETECTION (Perfect for Level 1 & 2)
// =========================================================

static BoundingBox find_grid_by_lines(gdImagePtr img) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    
    // 1. Horizontal & Vertical Projections
    int* h_proj = compute_horizontal_projection(img);
    int* v_proj = compute_vertical_projection(img);
    
    // 2. Look for long, solid lines (Grid borders)
    // A grid line will have black pixels across > 40% of the image width/height
    int border_thresh_w = width * 0.4;
    int border_thresh_h = height * 0.4;
    
    BoundingBox grid = {0,0,0,0};
    
    // Find Top Line
    for(int y=0; y<height; y++) {
        if(h_proj[y] > border_thresh_w) { grid.y = y; break; }
    }
    // Find Bottom Line
    for(int y=height-1; y>=0; y--) {
        if(h_proj[y] > border_thresh_w) { grid.height = y - grid.y; break; }
    }
    // Find Left Line
    for(int x=0; x<width; x++) {
        if(v_proj[x] > border_thresh_h) { grid.x = x; break; }
    }
    // Find Right Line
    for(int x=width-1; x>=0; x--) {
        if(v_proj[x] > border_thresh_h) { grid.width = x - grid.x; break; }
    }
    
    free(h_proj);
    free(v_proj);
    
    // Validation: A valid grid must be reasonably large
    if(grid.width < 50 || grid.height < 50) return (BoundingBox){0,0,0,0};
    
    return grid;
}

// =========================================================
// STRATEGY B: GEOMETRIC SMEARING (Fallback for Level 3)
// =========================================================

static BoundingBox get_component_bbox(gdImagePtr img, int x, int y, int** visited) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    int min_x = x, max_x = x, min_y = y, max_y = y;

    int stack_capacity = width * height;
    int* stack_x = (int*)malloc(stack_capacity * sizeof(int));
    int* stack_y = (int*)malloc(stack_capacity * sizeof(int));
    if (!stack_x || !stack_y) return (BoundingBox){0,0,0,0};
    
    int top = 0;
    stack_x[top] = x; stack_y[top] = y; top++;
    visited[y][x] = 1;
    int target = gdImageGetPixel(img, x, y);

    while (top > 0) {
        top--;
        int cx = stack_x[top]; int cy = stack_y[top];

        if (cx < min_x) min_x = cx;
        if (cx > max_x) max_x = cx;
        if (cy < min_y) min_y = cy;
        if (cy > max_y) max_y = cy;

        int dx[] = {1, -1, 0, 0}; int dy[] = {0, 0, 1, -1};
        for (int i = 0; i < 4; i++) {
            int nx = cx + dx[i]; int ny = cy + dy[i];
            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                if (!visited[ny][nx] && gdImageGetPixel(img, nx, ny) == target) {
                    visited[ny][nx] = 1;
                    stack_x[top] = nx; stack_y[top] = ny; top++;
                }
            }
        }
    }
    free(stack_x); free(stack_y);
    return (BoundingBox){min_x, min_y, max_x - min_x + 1, max_y - min_y + 1};
}

static void cut_long_lines(gdImagePtr img) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    int white = gdImageColorResolve(img, 255, 255, 255);
    int black = gdImageColorResolve(img, 0, 0, 0);
    int line_threshold = width * 0.4; 

    for (int y = 0; y < height; y++) {
        int run = 0, start = -1;
        for (int x = 0; x < width; x++) {
            if (gdImageGetPixel(img, x, y) == black) {
                if (start == -1) start = x;
                run++;
            } else {
                if (run > line_threshold) gdImageLine(img, start, y, x-1, y, white);
                run = 0; start = -1;
            }
        }
        if (run > line_threshold) gdImageLine(img, start, y, width-1, y, white);
    }
}

static BoundingBox refine_grid_region(gdImagePtr img, BoundingBox rough_grid) {
    int* h_proj = (int*)calloc(rough_grid.height, sizeof(int));
    for(int y=0; y < rough_grid.height; y++) 
        for(int x=0; x < rough_grid.width; x++) 
            if(gdImageGetPixel(img, rough_grid.x+x, rough_grid.y+y) == 0) h_proj[y]++;

    // 1. Cut Header (Top 25%)
    int header_cut_y = -1;
    int gap = 0;
    for(int y=0; y < rough_grid.height * 0.25; y++) {
        if(h_proj[y] < 5) gap++;
        else {
            if(gap > 10) header_cut_y = y; 
            gap = 0;
        }
    }

    if (header_cut_y != -1) {
        printf("  > [Refine] Cut HEADER at relative Y=%d\n", header_cut_y);
        rough_grid.y += header_cut_y;
        rough_grid.height -= header_cut_y;
    }

    // 2. Intelligent Footer Split
    int footer_cut_y = -1;
    gap = 0;
    int content_seen = 0;
    
    // Scan bottom 50%
    for(int y = rough_grid.height - 1; y > rough_grid.height * 0.5; y--) {
        int proj_idx = y + (header_cut_y != -1 ? header_cut_y : 0);
        if (proj_idx >= 0 && proj_idx < rough_grid.height) {
             if (h_proj[proj_idx] < 5) {
                gap++;
            } else {
                if (gap > 0) content_seen = 1;
                // If we see content, then a big gap (>20px), then content again
                if(content_seen && gap > 20) {
                    footer_cut_y = y + gap;
                    break; 
                }
                gap = 0;
            }
        }
    }

    if (footer_cut_y != -1) {
        printf("  > [Refine] Detected internal gap. Splitting Footer/Wordlist at relative Y=%d\n", footer_cut_y);
        rough_grid.height = footer_cut_y;
    }
    
    free(h_proj);
    return rough_grid;
}

static BoundingBox find_grid_advanced(gdImagePtr original_img) {
    int width = gdImageSX(original_img);
    int height = gdImageSY(original_img);
    
    // 1. Clean
    gdImagePtr clean = gdImageCreate(width, height);
    int white = gdImageColorAllocate(clean, 255, 255, 255);
    int black = gdImageColorAllocate(clean, 0, 0, 0);
    gdImageFilledRectangle(clean, 0, 0, width-1, height-1, white);
    
    int** visited = (int**)malloc(height * sizeof(int*));
    for(int i=0; i<height; i++) visited[i] = (int*)calloc(width, sizeof(int));
    
    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            if(!visited[y][x] && gdImageGetPixel(original_img, x, y) == 0) {
                BoundingBox bb = get_component_bbox(original_img, x, y, visited);
                int is_noise = (bb.width < 2 || bb.height < 2);
                int is_frame = (bb.width > width-20 || bb.height > height-20);
                if(!is_noise && !is_frame) {
                    gdImageFilledRectangle(clean, bb.x, bb.y, bb.x+bb.width-1, bb.y+bb.height-1, black);
                }
            }
        }
    }
    for(int i=0; i<height; i++) free(visited[i]);
    free(visited);
    
    // 2. Cut Lines
    cut_long_lines(clean);
    
    // 3. Smear (Optimized for Level 3)
    int h_smear = 50; 
    int v_smear = 30; 

    for (int y = 0; y < height; y++) {
        int last=-1;
        for(int x=0; x<width; x++) {
            if(gdImageGetPixel(clean, x, y)==black) {
                if(last!=-1 && x-last < h_smear) gdImageLine(clean, last, y, x, y, black);
                last=x;
            }
        }
    }
    for(int x=0; x<width; x++) {
        int last=-1;
        for(int y=0; y<height; y++) {
            if(gdImageGetPixel(clean, x, y)==black) {
                if(last!=-1 && y-last < v_smear) gdImageLine(clean, x, last, x, y, black);
                last=y;
            }
        }
    }
    
    visited = (int**)malloc(height * sizeof(int*));
    for(int i=0; i<height; i++) visited[i] = (int*)calloc(width, sizeof(int));
    
    BoundingBox best = {0,0,0,0};
    float max_score = 0;
    
    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            if(!visited[y][x] && gdImageGetPixel(clean, x, y) == black) {
                BoundingBox bb = get_component_bbox(clean, x, y, visited);
                int area = bb.width * bb.height;
                if(area < (width*height)/60) continue;
                
                float aspect = (float)bb.width / bb.height;
                float sq = 1.0f / (1.0f + fabs(aspect - 1.0f));
                float score = area * (sq * sq);
                
                if(score > max_score) { max_score = score; best = bb; }
            }
        }
    }
    
    for(int i=0; i<height; i++) free(visited[i]);
    free(visited);
    gdImageDestroy(clean);
    
    if(best.width > 0) best = refine_grid_region(original_img, best);
    
    // Add Padding
    if (best.width > 0) {
        int pad = 8;
        best.x = (best.x > pad) ? best.x - pad : 0;
        best.y = (best.y > pad) ? best.y - pad : 0;
        best.width += pad * 2;
        best.height += pad * 2;
        if(best.width+best.x > width) best.width = width-best.x;
        if(best.height+best.y > height) best.height = height-best.y;
    }
    
    return best;
}

// --- MAIN ENTRY POINT ---
BoundingBox find_grid_by_projection(gdImagePtr img) {
    // 1. Try to find solid grid lines (Level 1/2)
    printf("[Grid Detection] Attempting Strategy A: Line Detection...\n");
    BoundingBox line_grid = find_grid_by_lines(img);
    
    if (line_grid.width > 0) {
        printf("  > Strategy A successful! Found solid grid lines.\n");
        return line_grid;
    }
    
    // 2. Fallback to Smearing (Level 3)
    printf("  > Strategy A failed (No lines). Switching to Strategy B: Advanced Geometric...\n");
    return find_grid_advanced(img);
}

// Fallbacks
BoundingBox find_grid_by_text_clustering(gdImagePtr img) { return find_grid_by_projection(img); }
void flood_fill(gdImagePtr img, int x, int y, BoundingBox* bbox, int** visited) { (void)img; (void)x; (void)y; (void)bbox; (void)visited; }