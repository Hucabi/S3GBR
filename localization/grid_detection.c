#include "localization.h"
#include <math.h>

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

static int has_solid_grid_lines(gdImagePtr img) {
    int w = gdImageSX(img);
    int h = gdImageSY(img);
    int black = gdImageColorResolve(img, 0, 0, 0);
    int long_lines_found = 0;
    for (int y = 0; y < h; y += 10) {
        int run = 0;
        for (int x = 0; x < w; x++) {
            if (gdImageGetPixel(img, x, y) == black) {
                run++;
                if (run > w * 0.4) { long_lines_found++; break; }
            } else { run = 0; }
        }
        if (long_lines_found > 5) return 1; 
    }
    return 0;
}

static BoundingBox refine_grid_region(gdImagePtr img, BoundingBox rough_grid) {
    int* h_proj = (int*)calloc(rough_grid.height, sizeof(int));
    for(int y=0; y < rough_grid.height; y++) 
        for(int x=0; x < rough_grid.width; x++) 
            if(gdImageGetPixel(img, rough_grid.x+x, rough_grid.y+y) == 0) h_proj[y]++;

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
        rough_grid.y += header_cut_y;
        rough_grid.height -= header_cut_y;
    }

    int footer_cut_y = -1;
    gap = 0;
    int content_seen = 0;
    for(int y = rough_grid.height - 1; y > rough_grid.height * 0.6; y--) {
        int proj_idx = y + (header_cut_y != -1 ? header_cut_y : 0);
        if (proj_idx >= 0 && proj_idx < rough_grid.height) {
             if (h_proj[proj_idx] < 5) gap++;
             else {
                if (gap > 0) content_seen = 1;
                if(content_seen && gap > 50) { footer_cut_y = y + gap; break; }
                gap = 0;
            }
        }
    }
    if (footer_cut_y != -1) rough_grid.height = footer_cut_y;
    free(h_proj);
    return rough_grid;
}

BoundingBox find_grid_by_projection(gdImagePtr img) {
    int width = gdImageSX(img);
    int height = gdImageSY(img);
    gdImagePtr clean = gdImageCreate(width, height);
    int white = gdImageColorAllocate(clean, 255, 255, 255);
    int black = gdImageColorAllocate(clean, 0, 0, 0);
    gdImageFilledRectangle(clean, 0, 0, width-1, height-1, white);
    
    int** visited = (int**)malloc(height * sizeof(int*));
    for(int i=0; i<height; i++) visited[i] = (int*)calloc(width, sizeof(int));
    
    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            if(!visited[y][x] && gdImageGetPixel(img, x, y) == 0) {
                BoundingBox bb = get_component_bbox(img, x, y, visited);
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
    
    int has_lines = has_solid_grid_lines(clean);
    int h_smear = has_lines ? 5 : 55;
    int v_smear = has_lines ? 5 : 40;
    
    printf("  > GRID DETECTION / Lines: %s. Smear: %dx%d\n", has_lines ? "YES" : "NO", h_smear, v_smear);

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
                long area = (long)bb.width * bb.height;
                if(area < (width*height)/50) continue;
                float score = (float)area;
                if (bb.y > height * 0.7) score *= 0.5f; 
                if(score > max_score) { max_score = score; best = bb; }
            }
        }
    }
    
    for(int i=0; i<height; i++) free(visited[i]);
    free(visited);
    gdImageDestroy(clean);
    
    if(best.width > 0) best = refine_grid_region(img, best);
    if (best.width > 0) {
        int pad = 8;
        best.x = (best.x > pad) ? best.x - pad : 0;
        best.y = (best.y > pad) ? best.y - pad : 0;
        best.width += pad * 2;
        best.height += pad * 2;
    }
    return best;
}

BoundingBox find_grid_by_text_clustering(gdImagePtr img) { return find_grid_by_projection(img); }
void flood_fill(gdImagePtr img, int x, int y, BoundingBox* bbox, int** visited) { (void)img; (void)x; (void)y; (void)bbox; (void)visited; }