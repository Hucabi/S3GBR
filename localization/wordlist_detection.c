#include "localization.h"

// Calculate bp density in given rect
static float get_density(gdImagePtr img, BoundingBox box) {
    if (box.width <= 0 || box.height <= 0) return 0.0;
    int black_pixels = 0;
    int black = gdImageColorResolve(img, 0, 0, 0);

    for (int x = box.x; x < box.x + box.width; x++) {
        for (int y = box.y; y < box.y + box.height; y++) {
            if (x < gdImageSX(img) && y < gdImageSY(img)) {
                if (gdImageGetPixel(img, x, y) == black) {
                    black_pixels++;
                }
            }
        }
    }
    return (float)black_pixels / (box.width * box.height);
}

static BoundingBox tighten_box(gdImagePtr img, BoundingBox rough) {
    int min_x = rough.x + rough.width;
    int max_x = rough.x;
    int min_y = rough.y + rough.height;
    int max_y = rough.y;
    int found = 0;
    int black = gdImageColorResolve(img, 0, 0, 0);

    for (int x = rough.x; x < rough.x + rough.width; x++) {
        for (int y = rough.y; y < rough.y + rough.height; y++) {
            if (x < gdImageSX(img) && y < gdImageSY(img)) {
                if (gdImageGetPixel(img, x, y) == black) {
                    if (x < min_x) min_x = x;
                    if (x > max_x) max_x = x;
                    if (y < min_y) min_y = y;
                    if (y > max_y) max_y = y;
                    found = 1;
                }
            }
        }
    }

    if (!found) return (BoundingBox){0,0,0,0};
    
    int pad = 5;
    return (BoundingBox){
        min_x - pad, 
        min_y - pad, 
        (max_x - min_x) + (pad*2), 
        (max_y - min_y) + (pad*2)
    };
}

BoundingBox find_wordlist_region(gdImagePtr img, BoundingBox grid) {
    int w = gdImageSX(img);
    int h = gdImageSY(img);
    
    // candidate regions
    BoundingBox right_area = {
        grid.x + grid.width, 
        0, 
        w - (grid.x + grid.width), 
        h
    };
    
    BoundingBox bottom_area = {
        0, 
        grid.y + grid.height, 
        w, 
        h - (grid.y + grid.height)
    };
    
    BoundingBox left_area = {
        0, 
        0, 
        grid.x, 
        h
    };

    float d_right = get_density(img, right_area);
    float d_bottom = get_density(img, bottom_area);
    float d_left = get_density(img, left_area);

    printf("WORDLIST SEARCH / Densities - Right: %.4f, Bottom: %.4f, Left: %.4f\n", d_right, d_bottom, d_left);

    BoundingBox best_region = {0,0,0,0};

    // pick area with highest density
    if (d_right > 0.005 && d_right >= d_bottom && d_right >= d_left) {
        printf("  > Found Wordlist on RIGHT\n");
        best_region = right_area;
    } 
    else if (d_bottom > 0.005 && d_bottom >= d_right && d_bottom >= d_left) {
        printf("  > Found Wordlist on BOTTOM\n");
        best_region = bottom_area;
    }
    else if (d_left > 0.005) {
        printf("  > Found Wordlist on LEFT\n");
        best_region = left_area;
    }

    if (best_region.width > 0) {
        return tighten_box(img, best_region);
    }

    return (BoundingBox){0,0,0,0};
}