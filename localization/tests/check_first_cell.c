#include <gd.h>
#include <stdio.h>

int main() {
    FILE* fp = fopen("../data/images/level_1_binarized.png", "rb");
    gdImagePtr img = gdImageCreateFromPng(fp);
    fclose(fp);

    // Grid position
    int grid_x = 184;
    int grid_y = 18;

    // First cell: from (185, 19) to (218, 53)
    int cell_x = 185;
    int cell_y = 19;
    int cell_w = 34;
    int cell_h = 35;

    printf("First cell: (%d,%d) to (%d,%d)\n", cell_x, cell_y, cell_x+cell_w-1, cell_y+cell_h-1);

    // Check a few pixels in the first cell
    for (int y = cell_y; y < cell_y + 5; y++) {
        printf("Row %d: ", y);
        for (int x = cell_x; x < cell_x + 5; x++) {
            int color = gdImageGetPixel(img, x, y);
            printf("%3d ", color);
        }
        printf("...\n");
    }

    gdImageDestroy(img);
    return 0;
}