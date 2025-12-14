#include "localization.h"
#include <sys/stat.h>
#include <sys/types.h>

void save_debug_image(gdImagePtr img, char* filename,
		BoundingBox grid, BoundingBox wordlist) {
    int red = gdImageColorAllocate(img, 255, 0, 0);
    int green = gdImageColorAllocate(img, 0, 255, 0);
    
    // Draw grid in red
    gdImageRectangle(img, grid.x, grid.y, 
		    grid.x + grid.width, grid.y + grid.height, red);
    gdImageRectangle(img, grid.x+1, grid.y+1, grid.x + grid.width-1,
		    grid.y + grid.height-1, red); // Thicker

    // Draw worldlist in green
    if (wordlist.width > 0) {
        gdImageRectangle(img, wordlist.x, wordlist.y,
			wordlist.x + wordlist.width,
			wordlist.y + wordlist.height, green);
        gdImageRectangle(img, wordlist.x+1, wordlist.y+1,
			wordlist.x + wordlist.width-1,
			wordlist.y + wordlist.height-1, green);
    }

    FILE* out = fopen(filename, "wb");
    if (out) {
        gdImagePng(img, out);
        fclose(out);
        printf("Debug visualization saved: %s\n", filename);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <image_file>\n", argv[0]);
        return 1;
    }

    printf("### OCR Wordsearch - Localization & Extraction ###\n");
    printf("Input image: %s\n\n", argv[1]);

    FILE* in = fopen(argv[1], "rb");
    if (!in) {
        printf("Error: Could not open file %s\n", argv[1]);
        return 1;
    }
    gdImagePtr img = gdImageCreateFromPng(in);
    fclose(in);

    if (!img) {
        printf("Error: Could not load PNG image.\n");
        return 1;
    }

    printf("Image loaded: %dx%d pixels\n\n", gdImageSX(img), gdImageSY(img));

    printf("STEP 1 = Grid Detection\n");
    printf("-----------------------------------\n");
    BoundingBox grid = find_grid_by_projection(img);
    printf("Grid found: x=%d, y=%d, width=%d, height=%d\n\n", 
           grid.x, grid.y, grid.width, grid.height);

    printf("STEP 2 = Grid Letter Extraction\n");
    printf("-----------------------------------\n");
    if (grid.width > 0 && grid.height > 0) {
        extract_grid_letters(img, grid);
    } else {
        printf("Error: Invalid grid detection.\n");
    }
    printf("\n");

    printf("STEP 3 = Wordlist Detection\n");
    printf("-----------------------------------\n");
    BoundingBox wordlist = find_wordlist_region(img, grid);
    printf("Wordlist found: x=%d, y=%d, width=%d, height=%d\n\n", 
           wordlist.x, wordlist.y, wordlist.width, wordlist.height);

    printf("STEP 4 = Wordlist Letter Extraction\n");
    printf("-----------------------------------\n");
    if (wordlist.width > 0 && wordlist.height > 0) {
        extract_wordlist_letters(img, wordlist);
    } else {
        printf("Warning: No wordlist detected.\n");
    }
    printf("\n");

    printf("STEP 5 = Debug Visualization\n");
    printf("-----------------------------------\n");
    save_debug_image(img, "localization_debug.png", grid, wordlist);
    printf("\n");

    printf("=====================================\n");
    printf("      EXTRACTION COMPLETE      \n");
    printf("=====================================\n");
    printf("Grid cells (32x32):    ../data/grid/cells/\n");
    printf("Wordlist letters (28x28): ../data/wordlist/cells/\n");
    printf("Debug visualization:   localization_debug.png\n");

    gdImageDestroy(img);
    return 0;
}
