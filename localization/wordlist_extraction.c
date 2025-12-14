#include "localization.h"
#include <sys/stat.h>
#include <sys/types.h>

// --- Helper: Save 28x28 Centered (From your provided code) ---
void save_letter_28x28(gdImagePtr img, int x, int y, int w, int h, const char* filename) {
    // Filter tiny noise
    if (w < 2 || h < 2) return;

    // 1. Create 28x28 Canvas (White Background)
    gdImagePtr dest = gdImageCreateTrueColor(28, 28);
    int white = gdImageColorAllocate(dest, 255, 255, 255);
    gdImageFilledRectangle(dest, 0, 0, 27, 27, white);

    // 2. Calculate Scaling (Fit to 20x20 box inside 28x28)
    int target_size = 20;
    int new_w, new_h;
    
    if (w > h) {
        new_w = target_size;
        new_h = (int)((float)h / w * target_size);
    } else {
        new_h = target_size;
        new_w = (int)((float)w / h * target_size);
    }
    
    if (new_w < 1) new_w = 1;
    if (new_h < 1) new_h = 1;

    // 3. Center it
    int dest_x = (28 - new_w) / 2;
    int dest_y = (28 - new_h) / 2;

    // 4. Resample
    gdImageCopyResampled(dest, img, dest_x, dest_y, x, y, new_w, new_h, w, h);
    
    // 5. Save
    FILE* out = fopen(filename, "wb");
    if (out) {
        gdImagePng(dest, out);
        fclose(out);
    }
    gdImageDestroy(dest);
}

// --- Main Extraction ---
void extract_wordlist_letters(gdImagePtr img, BoundingBox box) {
    printf("[Extraction] Extracting Wordlist Letters...\n");
    
    // Create base directory
    mkdir("../data/wordlist/cells", 0777);

    // Crop to the Wordlist Region
    gdImagePtr list_img = gdImageCreate(box.width, box.height);
    gdImageCopy(list_img, img, 0, 0, box.x, box.y, box.width, box.height);
    int black = gdImageColorResolve(list_img, 0, 0, 0);
    int w = gdImageSX(list_img);
    int h = gdImageSY(list_img);

    // 1. DETECT COLUMNS (Handles Level 3 multi-column layout)
    int* v_proj = (int*)calloc(w, sizeof(int));
    for(int x=0; x<w; x++) 
        for(int y=0; y<h; y++) 
            if(gdImageGetPixel(list_img, x, y) == black) v_proj[x]++;

    int col_starts[10];
    int col_ends[10];
    int num_cols = 0;
    int inside_col = 0;
    
    for(int x=0; x<w; x++) {
        if(v_proj[x] > 1) { // Threshold > 1 to ignore noise
            if(!inside_col) {
                if(num_cols < 10) col_starts[num_cols] = x;
                inside_col = 1;
            }
        } else {
            if(inside_col) {
                // Check if gap is significant (>10px) to separate columns
                int gap_w = 0;
                int gx = x;
                while(gx < w && v_proj[gx] <= 1) { gap_w++; gx++; }
                
                if(gap_w > 10 || gx == w) {
                    if(num_cols < 10) {
                        col_ends[num_cols] = x;
                        num_cols++;
                    }
                    inside_col = 0;
                }
            }
        }
    }
    if(inside_col && num_cols < 10) col_ends[num_cols++] = w;

    int word_count = 0;

    // 2. PROCESS EACH COLUMN
    for(int c=0; c<num_cols; c++) {
        int cx = col_starts[c];
        int cw = col_ends[c] - cx;
        if(cw < 5) continue;

        // 3. DETECT WORDS (Horizontal Projection within column)
        int* h_proj = (int*)calloc(h, sizeof(int));
        for(int y=0; y<h; y++) 
            for(int x=cx; x<cx+cw; x++) 
                if(gdImageGetPixel(list_img, x, y) == black) h_proj[y]++;

        int in_word = 0;
        int word_y = 0;

        for(int y=0; y<h; y++) {
            if(h_proj[y] > 1) {
                if(!in_word) {
                    word_y = y;
                    in_word = 1;
                }
            } else {
                if(in_word) {
                    int word_h = y - word_y;
                    
                    // Create directory for this word: ../data/wordlist/cells/word_0/
                    char word_dir[256];
                    sprintf(word_dir, "../data/wordlist/cells/word_%d", word_count);
                    mkdir(word_dir, 0777);

                    // 4. SEGMENT WORD INTO LETTERS
                    // We extract the word strip first to simplify projection
                    gdImagePtr word_strip = gdImageCreate(cw, word_h);
                    gdImageCopy(word_strip, list_img, 0, 0, cx, word_y, cw, word_h);
                    int ws_black = gdImageColorResolve(word_strip, 0, 0, 0);

                    // Vertical Projection on the word strip
                    int* let_proj = (int*)calloc(cw, sizeof(int));
                    for(int lx=0; lx<cw; lx++) 
                        for(int ly=0; ly<word_h; ly++) 
                            if(gdImageGetPixel(word_strip, lx, ly) == ws_black) let_proj[lx]++;

                    int in_letter = 0;
                    int let_x = 0;
                    int letter_idx = 0;

                    for(int lx=0; lx<cw; lx++) {
                        if(let_proj[lx] > 0) { // Any ink
                            if(!in_letter) {
                                let_x = lx;
                                in_letter = 1;
                            }
                        } else {
                            if(in_letter) {
                                // Save Letter
                                char fname[512];
                                sprintf(fname, "%s/letter_%d.png", word_dir, letter_idx++);
                                save_letter_28x28(word_strip, let_x, 0, lx-let_x, word_h, fname);
                                in_letter = 0;
                            }
                        }
                    }
                    if(in_letter) {
                        char fname[512];
                        sprintf(fname, "%s/letter_%d.png", word_dir, letter_idx++);
                        save_letter_28x28(word_strip, let_x, 0, cw-let_x, word_h, fname);
                    }

                    free(let_proj);
                    gdImageDestroy(word_strip);
                    word_count++;
                    in_word = 0;
                }
            }
        }
        free(h_proj);
    }
    
    free(v_proj);
    gdImageDestroy(list_img);
    printf("Extracted %d words to ../data/wordlist/cells/\n", word_count);
}