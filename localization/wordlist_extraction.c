#include "localization.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <math.h>

static void save_for_cnn(gdImagePtr src,
		int x, int y, int w, int h, const char* filename) {
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

static void process_and_save_blob(gdImagePtr strip,
		int x, int y, int w, int h, char* base_dir, int* idx) {
    float aspect = (float)w / h;

    int do_split = 0;
    int split_x = -1;

    int search_start = w * 0.15;
    int search_end = w * 0.85;
    
    // safety clamps
    if (search_start < 1) search_start = 1;
    if (search_end > w - 2) search_end = w - 2;

    if (aspect > 0.90) { // Look at anything wider than a I
        int* proj = (int*)calloc(w, sizeof(int));
        int black = gdImageColorResolve(strip, 0, 0, 0);
        
        // vertical projection
        for (int ix = 0; ix < w; ix++) {
            for (int iy = 0; iy < h; iy++) {
                if (gdImageGetPixel(strip, x + ix, y + iy) == black) {
                    proj[ix]++;
                }
            }
        }

        // find the thinest point = valley
        int min_ink = INT_MAX;
        if (search_start <= search_end) {
            for (int ix = search_start; ix <= search_end; ix++) {
                if (proj[ix] < min_ink) {
                    min_ink = proj[ix];
                    split_x = ix;
                }
            }
        }
        free(proj);

        if (split_x != -1) {
            float ink_ratio = (float)min_ink / h;
            if (aspect > 1.2 && ink_ratio < 0.50) {
                do_split = 1;
            }
            else if (aspect > 0.90 && ink_ratio < 0.22) {
                do_split = 1;
            }
        }
    }

    if (do_split) {
        process_and_save_blob(strip, x, y, split_x, h, base_dir, idx);
        process_and_save_blob(strip, x + split_x, y, w - split_x, h,
			base_dir, idx);
    } else {
        if(w > 2 && h > 5) {
            char fname[512];
            sprintf(fname, "%s/letter_%d.png", base_dir, (*idx)++);
            save_for_cnn(strip, x, y, w, h, fname);
        }
    }
}

typedef struct { int x, y, w, h; } Blob;

static int compare_x(const void* a, const void* b) {
    return ((Blob*)a)->x - ((Blob*)b)->x;
}

void extract_wordlist_letters(gdImagePtr img, BoundingBox box) {
    printf("Extraction...\n");
    mkdir("..data/wordlist/cells", 0777);

    gdImagePtr list_img = gdImageCreate(box.width, box.height);
    gdImageCopy(list_img, img, 0, 0, box.x, box.y, box.width, box.height);
    int black = gdImageColorResolve(list_img, 0, 0, 0);
    int w = gdImageSX(list_img);
    int h = gdImageSY(list_img);

    int word_count = 0;

    int* h_proj = calloc(h, sizeof(int));
    for(int y=0; y<h; y++) {
        for(int x=0; x<w; x++) {
            if(gdImageGetPixel(list_img, x, y) == black) h_proj[y]++;
        }
    }
    
    int in_row = 0;
    int wy = 0;
    int noise_floor = 2; 

    for(int y=0; y<h; y++) {
        int is_ink = (h_proj[y] > noise_floor);
        int is_last = (y == h - 1);

        if(is_ink) { 
            if(!in_row) { wy = y; in_row = 1; }
            if (is_last && in_row) { /* fallthrough */ } 
            else { continue; }
        } 
        
        if(in_row && (!is_ink || is_last)) {
            // Found a row
            int wh = (is_last && is_ink) ? (y - wy + 1) : (y - wy);
            in_row = 0;

            if (wh < 8) continue;

            gdImagePtr strip = gdImageCreate(w, wh);
            gdImageCopy(strip, list_img, 0, 0, 0, wy, w, wh);
            
            // fLOOD fILL
            int* visited = calloc(w * wh, sizeof(int));
            Blob blobs[200];
            int b_count = 0;
            
            for(int by=0; by<wh; by++) {
                for(int bx=0; bx<w; bx++) {
                    if(!visited[by*w+bx] && gdImageGetPixel(strip, bx, by)
				    ==black) {
                        int min_x=bx, max_x=bx, min_y=by, max_y=by;
                        
                        int* stack = malloc(w*wh*2*sizeof(int));
                        int top=0;
                        stack[top++] = bx; stack[top++] = by;
                        visited[by*w+bx] = 1;
                        
                        while(top>0) {
                            int cur_y = stack[--top]; 
                            int cur_x = stack[--top];
                            
                            if(cur_x < min_x) min_x = cur_x; 
                            if(cur_x > max_x) max_x = cur_x;
                            if(cur_y < min_y) min_y = cur_y; 
                            if(cur_y > max_y) max_y = cur_y;
                            
                            int dx[]={1,-1,0,0}, dy[]={0,0,1,-1};
                            for(int i=0; i<4; i++) {
                                int nx=cur_x+dx[i], ny=cur_y+dy[i];
                                if(nx>=0 && nx<w && ny>=0 && ny<wh) {
                                    if(!visited[ny*w+nx] &&
						    gdImageGetPixel
						    (strip, nx, ny)==black) {
                                        visited[ny*w+nx]=1;
                                        stack[top++]=nx; stack[top++]=ny;
                                    }
                                }
                            }
                        }
                        free(stack);
                        if (b_count < 200) blobs[b_count++] =
				(Blob){min_x, min_y, max_x-min_x+1,
					max_y-min_y+1};
                    }
                }
            }
            free(visited);

            // merge vertical blobs 
            for(int i=0; i<b_count; i++) {
                if(blobs[i].w == 0) continue; 
                for(int j=i+1; j<b_count; j++) {
                    if(blobs[j].w == 0) continue;
                    int overlap = (blobs[i].x < blobs[j].x
				    + blobs[j].w && blobs[i].x 
				    + blobs[i].w > blobs[j].x);
                    if(overlap) {
                        int new_min_x = (blobs[i].x < blobs[j].x) 
				? blobs[i].x : blobs[j].x;
                        int new_min_y = (blobs[i].y < blobs[j].y) 
				? blobs[i].y : blobs[j].y;
                        int new_max_x = (blobs[i].x+blobs[i].w > 
					blobs[j].x+blobs[j].w) ? 
					blobs[i].x+blobs[i].w : 
					blobs[j].x+blobs[j].w;
                        int new_max_y = (blobs[i].y+blobs[i].h > 
					blobs[j].y+blobs[j].h) ? 
					blobs[i].y+blobs[i].h : 
					blobs[j].y+blobs[j].h;
                        blobs[i].x = new_min_x; blobs[i].y = new_min_y;
                        blobs[i].w = new_max_x - new_min_x; blobs[i].h = 
				new_max_y - new_min_y;
                        blobs[j].w = 0; 
                    }
                }
            }

            qsort(blobs, b_count, sizeof(Blob), compare_x);

            char w_dir[256];
            sprintf(w_dir, "../data/wordlist/cells/word_%d", word_count++);
            mkdir(w_dir, 0777);
            
            int l_idx = 0;
            for(int i=0; i<b_count; i++) {
                if(blobs[i].w > 0) {
                    process_and_save_blob(strip, blobs[i].x, 
				    blobs[i].y, blobs[i].w, 
				    blobs[i].h, w_dir, &l_idx);
                }
            }
            
            gdImageDestroy(strip);
        }
    }
    
    free(h_proj);
    gdImageDestroy(list_img);
    printf("Extracted %d words via Row-Based Analysis.\n", word_count);
}
