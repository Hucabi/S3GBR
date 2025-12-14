#include "localization.h"
#include <math.h>

// =========================================================
// PURE BLOB STRATEGY (For inputs with NO lines)
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

// --- Split merged blobs (Grid + Wordlist) ---
static BoundingBox refine_grid_region(gdImagePtr img, BoundingBox rough_grid) {
    int* h_proj = (int*)calloc(rough_grid.height, sizeof(int));
    for(int y=0; y < rough_grid.height; y++) 
        for(int x=0; x < rough_grid.width; x++) 
            if(gdImageGetPixel(img, rough_grid.x+x, rough_grid.y+y) == 0) h_proj[y]++;

    // 1. Cut Header (Top 20%)
    // [FIX] Increased gap threshold from 10 to 45.
    // This prevents the code from thinking the space between Row 1 and Row 2 is a header separator.
    int header_cut_y = -1;
    int gap = 0;
    for(int y=0; y < rough_grid.height * 0.20; y++) {
        if(h_proj[y] < 5) gap++;
        else {
            if(gap > 45) header_cut_y = y; 
            gap = 0;
        }
    }

    if (header_cut_y != -1) {
        printf("  > [Refine] Cut HEADER at relative Y=%d\n", header_cut_y);
        rough_grid.y += header_cut_y;
        rough_grid.height -= header_cut_y;
    }

    // 2. Split Footer (Bottom 40%)
    // [FIX] Increased gap threshold from 20 to 50.
    // This prevents cutting off the bottom rows due to line spacing.
    int footer_cut_y = -1;
    gap = 0;
    int content_seen = 0;
    
    for(int y = rough_grid.height - 1; y > rough_grid.height * 0.6; y--) {
        int proj_idx = y + (header_cut_y != -1 ? header_cut_y : 0);
        if (proj_idx >= 0 && proj_idx < rough_grid.height) {
             if (h_proj[proj_idx] < 5) {
                gap++;
            } else {
                if (gap > 0) content_seen = 1;
                
                // Gap must be huge (>50px) to consider it a split
                if(content_seen && gap > 50) {
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
    
    gdImagePtr clean = gdImageCreate(width, height);
    int white = gdImageColorAllocate(clean, 255, 255, 255);
    int black = gdImageColorAllocate(clean, 0, 0, 0);
    gdImageFilledRectangle(clean, 0, 0, width-1, height-1, white);
    
    int** visited = (int**)malloc(height * sizeof(int*));
    for(int i=0; i<height; i++) visited[i] = (int*)calloc(width, sizeof(int));
    
    // 1. Clean Noise (Filter tiny specks)
    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            if(!visited[y][x] && gdImageGetPixel(original_img, x, y) == 0) {
                BoundingBox bb = get_component_bbox(original_img, x, y, visited);
                int is_noise = (bb.width < 5 || bb.height < 5); 
                int is_frame = (bb.width > width-10 || bb.height > height-10); 
                if(!is_noise && !is_frame) {
                    gdImageFilledRectangle(clean, bb.x, bb.y, bb.x+bb.width-1, bb.y+bb.height-1, black);
                }
            }
        }
    }
    for(int i=0; i<height; i++) free(visited[i]);
    free(visited);
    
    // 2. Smear (Connect letters into blocks)
    // [FIX] Increased smear values to ensure the Grid becomes one solid object
    int h_smear = 55; 
    int v_smear = 40; 

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
    
    // 3. Score Blobs to find the Grid
    visited = (int**)malloc(height * sizeof(int*));
    for(int i=0; i<height; i++) visited[i] = (int*)calloc(width, sizeof(int));
    
    BoundingBox best = {0,0,0,0};
    float max_score = 0;
    
    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            if(!visited[y][x] && gdImageGetPixel(clean, x, y) == black) {
                BoundingBox bb = get_component_bbox(clean, x, y, visited);
                long area = (long)bb.width * bb.height;
                
                // IGNORE SMALL BLOBS (Must be at least 5% of the screen area roughly)
                if(area < (width*height)/50) continue;
                
                // [FIX] SCORE BY AREA ONLY
                float score = (float)area;
                
                if (bb.y > height * 0.7) score *= 0.5f;

                printf("  Blob found: %dx%d at Y=%d. Area=%ld. Score=%.0f\n", 
                       bb.width, bb.height, bb.y, area, score);
                
                if(score > max_score) { max_score = score; best = bb; }
            }
        }
    }
    
    for(int i=0; i<height; i++) free(visited[i]);
    free(visited);
    gdImageDestroy(clean);
    
    // [FIX] Ensure we refine (remove noise) but respect the new threshold logic
    if(best.width > 0) best = refine_grid_region(original_img, best);
    
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

BoundingBox find_grid_by_projection(gdImagePtr img) {
    printf("[Grid Detection] Running Blob Analysis (No Lines Mode)...\n");
    return find_grid_advanced(img);
}

// Fallbacks
BoundingBox find_grid_by_text_clustering(gdImagePtr img) { return find_grid_by_projection(img); }
void flood_fill(gdImagePtr img, int x, int y, BoundingBox* bbox, int** visited) { (void)img; (void)x; (void)y; (void)bbox; (void)visited; }