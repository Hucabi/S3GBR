#include "localization.h"
#include <sys/stat.h>
#include <sys/types.h>

// --- Helper: Standardize to 28x28 (MNIST style) ---
static void save_28x28_normalized(gdImagePtr src, int x, int y, int w, int h, const char* filename) {
    // 1. Create 28x28 Canvas
    gdImagePtr dest = gdImageCreateTrueColor(28, 28);
    int white = gdImageColorAllocate(dest, 255, 255, 255);
    gdImageFilledRectangle(dest, 0, 0, 27, 27, white);

    // 2. Calculate Scaling (Fit to 20x20 box inside 28x28)
    int target_dim = 20;
    int new_w, new_h;
    
    if (w > h) {
        new_w = target_dim;
        new_h = (int)((float)h / w * target_dim);
    } else {
        new_h = target_dim;
        new_w = (int)((float)w / h * target_dim);
    }
    
    // Safety check to prevent 0 dimensions
    if (new_w < 1) new_w = 1;
    if (new_h < 1) new_h = 1;

    // 3. Center it
    int dest_x = (28 - new_w) / 2;
    int dest_y = (28 - new_h) / 2;

    // 4. Copy and Resize
    gdImageCopyResampled(dest, src, dest_x, dest_y, x, y, new_w, new_h, w, h);

    // 5. Save
    FILE* out = fopen(filename, "wb");
    if (out) {
        gdImagePng(dest, out);
        fclose(out);
    }
    gdImageDestroy(dest);
}

// --- Helper: Clean lines for analysis ---
static gdImagePtr create_clean_grid_copy(gdImagePtr src) {
    int w = gdImageSX(src);
    int h = gdImageSY(src);
    gdImagePtr clean = gdImageCreate(w, h);
    gdImageCopy(clean, src, 0, 0, 0, 0, w, h);
    int white = gdImageColorResolve(clean, 255, 255, 255);
    int black = gdImageColorResolve(clean, 0, 0, 0);

    for (int y = 0; y < h; y++) {
        int run = 0;
        for (int x = 0; x < w; x++) {
            if (gdImageGetPixel(clean, x, y) == black) run++;
            else run = 0;
            if (run > w * 0.5) { gdImageLine(clean, x-run, y, w, y, white); break; }
        }
    }
    for (int x = 0; x < w; x++) {
        int run = 0;
        for (int y = 0; y < h; y++) {
            if (gdImageGetPixel(clean, x, y) == black) run++;
            else run = 0;
            if (run > h * 0.5) { gdImageLine(clean, x, y-run, x, h, white); break; }
        }
    }
    return clean;
}

// --- Extraction Logic ---
void extract_grid_letters(gdImagePtr img, BoundingBox grid_box) {
    printf("[Extraction] Analyzing Grid Structure...\n");

    gdImagePtr grid_img = gdImageCreate(grid_box.width, grid_box.height);
    gdImageCopy(grid_img, img, 0, 0, grid_box.x, grid_box.y, grid_box.width, grid_box.height);

    gdImagePtr clean_img = create_clean_grid_copy(grid_img);
    int w = gdImageSX(clean_img);
    int h = gdImageSY(clean_img);
    int black = gdImageColorResolve(grid_img, 0, 0, 0);

    // Horizontal Projection
    int* h_proj = (int*)calloc(h, sizeof(int));
    for (int y = 0; y < h; y++) 
        for (int x = 0; x < w; x++) 
            if (gdImageGetPixel(clean_img, x, y) == black) h_proj[y]++;

    int* row_cuts = malloc(sizeof(int) * h);
    int num_rows = 0;
    int in_gap = 1;
    row_cuts[num_rows++] = 0;
    int gap_thresh = 5; 
    
    for (int y = 0; y < h; y++) {
        if (h_proj[y] <= gap_thresh) { 
            if (!in_gap) in_gap = 1;
        } else { 
            if (in_gap) {
                if (y > row_cuts[num_rows-1] + 5) row_cuts[num_rows++] = y - 2; 
                in_gap = 0;
            }
        }
    }
    row_cuts[num_rows++] = h; 

    // Vertical Projection
    int* v_proj = (int*)calloc(w, sizeof(int));
    for (int x = 0; x < w; x++) 
        for (int y = 0; y < h; y++) 
            if (gdImageGetPixel(clean_img, x, y) == black) v_proj[x]++;

    int* col_cuts = malloc(sizeof(int) * w);
    int num_cols = 0;
    in_gap = 1;
    col_cuts[num_cols++] = 0;

    for (int x = 0; x < w; x++) {
        if (v_proj[x] <= gap_thresh) { 
            if (!in_gap) in_gap = 1;
        } else {
            if (in_gap) {
                if (x > col_cuts[num_cols-1] + 5) col_cuts[num_cols++] = x - 2; 
                in_gap = 0;
            }
        }
    }
    col_cuts[num_cols++] = w;

    printf("  > Detected %d rows, %d col separators\n", num_rows-1, num_cols-1);

    // Extract Cells
    mkdir("../data/grid/cells", 0777);
    int cell_count = 0;

    for (int r = 1; r < num_rows; r++) {
        for (int c = 1; c < num_cols; c++) {
            int y1 = row_cuts[r-1];
            int y2 = row_cuts[r];
            int x1 = col_cuts[c-1];
            int x2 = col_cuts[c];
            
            int cell_w = x2 - x1;
            int cell_h = y2 - y1;
            
            int ink = 0;
            for(int cx=0; cx<cell_w; cx++)
                for(int cy=0; cy<cell_h; cy++)
                    if(gdImageGetPixel(grid_img, x1+cx, y1+cy) == black) ink++;

            if (cell_w > 5 && cell_h > 5 && ink > 10) {
                char filename[64];
                sprintf(filename, "../data/grid/cells/cell_%d_%d.png", r-1, c-1);
                
                // USE NEW 28x28 FUNCTION
                save_28x28_normalized(grid_img, x1, y1, cell_w, cell_h, filename);
                cell_count++;
            }
        }
    }

    printf("✓ Saved %d grid letters to ../data/grid/cells/\n", cell_count);

    free(h_proj);
    free(v_proj);
    free(row_cuts);
    free(col_cuts);
    gdImageDestroy(clean_img);
    gdImageDestroy(grid_img);
}