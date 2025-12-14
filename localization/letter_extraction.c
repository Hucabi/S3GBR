#include "localization.h"
#include <sys/stat.h>
#include <sys/types.h>

// save 28x28 centered 
static void save_for_cnn(gdImagePtr src, int x, int y, int w, int h,
		const char* filename) {
    if (w < 2 || h < 2) return;
    gdImagePtr dest = gdImageCreateTrueColor(28, 28);
    int white = gdImageColorAllocate(dest, 255, 255, 255);
    gdImageFilledRectangle(dest, 0, 0, 27, 27, white);

    int target = 20;
    int new_w, new_h;
    if (w > h) { 
        new_w = target; 
        new_h = (int)((float)h / w * target); 
    } else { 
        new_h = target; 
        new_w = (int)((float)w / h * target); 
    }
    
    if (new_w < 1) new_w = 1; 
    if (new_h < 1) new_h = 1;

    int dest_x = (28 - new_w) / 2;
    int dest_y = (28 - new_h) / 2;
    gdImageCopyResampled(dest, src, dest_x, dest_y, x, y, new_w, new_h, w, h);
    
    FILE* out = fopen(filename, "wb");
    if (out) { gdImagePng(dest, out); fclose(out); }
    gdImageDestroy(dest);
}

static gdImagePtr remove_grid_lines(gdImagePtr src) {
    int w = gdImageSX(src);
    int h = gdImageSY(src);
    gdImagePtr clean = gdImageCreate(w, h);
    gdImageCopy(clean, src, 0, 0, 0, 0, w, h);
    int white = gdImageColorResolve(clean, 255, 255, 255);
    int black = gdImageColorResolve(clean, 0, 0, 0);

    // (> 20% of dimension)
    for (int y = 0; y < h; y++) {
        int run = 0;
        for (int x = 0; x < w; x++) {
            if (gdImageGetPixel(clean, x, y) == black) run++;
            else run = 0;
            if (run > w * 0.2)
	    { gdImageLine(clean, x-run, y, w, y, white); break; }
        }
    }
    for (int x = 0; x < w; x++) {
        int run = 0;
        for (int y = 0; y < h; y++) {
            if (gdImageGetPixel(clean, x, y) == black) run++;
            else run = 0;
            if (run > h * 0.2)
	    { gdImageLine(clean, x, y-run, x, h, white); break; }
        }
    }
    return clean;
}

typedef struct {
    int x, y, w, h;
} Blob;

int compare_blobs(const void* a, const void* b) {
    Blob* blobA = (Blob*)a;
    Blob* blobB = (Blob*)b;
    
    // Determine if they are on the "same row" 
    int y_diff = abs(blobA->y - blobB->y);
    if (y_diff > 20) {
        return blobA->y - blobB->y; 
    } else {
        return blobA->x - blobB->x; 
    }
}

void find_blob(gdImagePtr img, int x, int y, int* visited, int w, int h,
		Blob* b) {
    // iterative because recursive cause problems
    int* stack_x = malloc(w * h * sizeof(int));
    int* stack_y = malloc(w * h * sizeof(int));
    int top = 0;
    
    stack_x[top] = x; stack_y[top] = y; top++;
    visited[y * w + x] = 1;
    
    int min_x = x, max_x = x, min_y = y, max_y = y;
    int black = gdImageColorResolve(img, 0, 0, 0);

    while(top > 0) {
        top--;
        int cx = stack_x[top]; 
        int cy = stack_y[top];
        
        if (cx < min_x) min_x = cx;
        if (cx > max_x) max_x = cx;
        if (cy < min_y) min_y = cy;
        if (cy > max_y) max_y = cy;
        
        int dx[] = {1, -1, 0, 0};
        int dy[] = {0, 0, 1, -1};
        
        for(int i=0; i<4; i++) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            
            if(nx >= 0 && nx < w && ny >= 0 && ny < h) {
                if(!visited[ny * w + nx] && gdImageGetPixel(img, nx, ny)
				== black) {
                    visited[ny * w + nx] = 1;
                    stack_x[top] = nx; stack_y[top] = ny; top++;
                }
            }
        }
    }
    
    b->x = min_x;
    b->y = min_y;
    b->w = max_x - min_x + 1;
    b->h = max_y - min_y + 1;
    
    free(stack_x);
    free(stack_y);
}

void extract_grid_letters(gdImagePtr img, BoundingBox grid_box) {
    printf("Extraction...\n");
    mkdir("data", 0777);
    mkdir("data/grid", 0777);
    mkdir("data/grid/cells", 0777);

    gdImagePtr grid_img = gdImageCreate(grid_box.width, grid_box.height);
    gdImageCopy(grid_img, img, 0, 0,
		    grid_box.x, grid_box.y, grid_box.width, grid_box.height);
    
    gdImagePtr clean = remove_grid_lines(grid_img);
    int w = gdImageSX(clean);
    int h = gdImageSY(clean);
    int black = gdImageColorResolve(clean, 0, 0, 0);

    int* visited = calloc(w * h, sizeof(int));
    Blob blobs[1000]; 
    int blob_count = 0;

    for(int y=0; y<h; y++) {
        for(int x=0; x<w; x++) {
            if(!visited[y*w + x] && gdImageGetPixel(clean, x, y) == black) {
                Blob b;
                find_blob(clean, x, y, visited, w, h, &b);
                
                // Filter Noise
                if (b.w > 5 && b.h > 5 && b.w < w/5 && b.h < h/5) {
                    if (blob_count < 1000) blobs[blob_count++] = b;
                }
            }
        }
    }

    qsort(blobs, blob_count, sizeof(Blob), compare_blobs);
    
    printf("  > Found %d distinct letter blobs.\n", blob_count);

    for(int i=0; i<blob_count; i++) {
        char fname[64];
        sprintf(fname, "data/grid/cells/cell_%d.png", i);
        save_for_cnn(clean,
			blobs[i].x, blobs[i].y, blobs[i].w, blobs[i].h, fname);
    }
    
    free(visited);
    gdImageDestroy(clean);
    gdImageDestroy(grid_img);
}
