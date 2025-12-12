// tests/check_cell_black.c
#include <gd.h>
#include <stdio.h>

int main() {
    FILE* fp = fopen("../data/images/level_1_binarized.png", "rb");
    gdImagePtr img = gdImageCreateFromPng(fp);
    fclose(fp);

    int cell_x = 185;
    int cell_y = 19;
    int cell_w = 34;
    int cell_h = 35;

    int black_count = 0;
    for (int y = cell_y; y < cell_y + cell_h; y++) {
        for (int x = cell_x; x < cell_x + cell_w; x++) {
            int color = gdImageGetPixel(img, x, y);
            if (color == 0) {
                black_count++;
            }
        }
    }

    printf("Black pixels in cell: %d out of %d\n", black_count, cell_w * cell_h);

    // Check if the letter is present by looking at the center
    int center_x = cell_x + cell_w / 2;
    int center_y = cell_y + cell_h / 2;
    printf("Center pixel (%d,%d): %d\n", center_x, center_y, gdImageGetPixel(img, center_x, center_y));

    gdImageDestroy(img);
    return 0;
}