#include "localization.h"

int* compute_vertical_projection(gdImagePtr img) {
    return compute_vertical_projection_within(img, (BoundingBox){0, 0, gdImageSX(img), gdImageSY(img)});
}

int* compute_horizontal_projection(gdImagePtr img) {
    return compute_horizontal_projection_within(img, (BoundingBox){0, 0, gdImageSX(img), gdImageSY(img)});
}

int* compute_vertical_projection_within(gdImagePtr img, BoundingBox area) {
    int* proj = (int*)calloc(area.width, sizeof(int));
    for (int x = 0; x < area.width; x++) {
        for (int y = 0; y < area.height; y++) {
            int px = area.x + x;
            int py = area.y + y;
            if (px < gdImageSX(img) && py < gdImageSY(img)) {
                if (gdImageGetPixel(img, px, py) == 0) { // Check for black pixel
                    proj[x]++;
                }
            }
        }
    }
    return proj;
}

int* compute_horizontal_projection_within(gdImagePtr img, BoundingBox area) {
    int* proj = (int*)calloc(area.height, sizeof(int));
    for (int y = 0; y < area.height; y++) {
        for (int x = 0; x < area.width; x++) {
            int px = area.x + x;
            int py = area.y + y;
            if (px < gdImageSX(img) && py < gdImageSY(img)) {
                if (gdImageGetPixel(img, px, py) == 0) {
                    proj[y]++;
                }
            }
        }
    }
    return proj;
}