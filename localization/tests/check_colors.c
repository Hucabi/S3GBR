// tests/check_colors.c
#include <gd.h>
#include <stdio.h>

int main() {
    FILE* fp = fopen("../data/images/level_1_binarized.png", "rb");
    gdImagePtr img = gdImageCreateFromPng(fp);
    fclose(fp);
    
    printf("Image info:\n");
    printf("  Size: %dx%d\n", gdImageSX(img), gdImageSY(img));
    printf("  TrueColor: %s\n", gdImageTrueColor(img) ? "Yes" : "No");
    printf("  Colors total: %d\n", gdImageColorsTotal(img));
    
    // Check color of index 0 and 255
    printf("\nColor palette (first 10 colors):\n");
    for (int i = 0; i < 10 && i < gdImageColorsTotal(img); i++) {
        int r = gdImageRed(img, i);
        int g = gdImageGreen(img, i);
        int b = gdImageBlue(img, i);
        printf("  Color %d: RGB(%d,%d,%d)\n", i, r, g, b);
    }
    
    // Check some specific points
    printf("\nSpecific points:\n");
    int points[][2] = {{0,0}, {184,18}, {185,19}, {200,30}, {10,100}};
    
    for (int i = 0; i < sizeof(points)/sizeof(points[0]); i++) {
        int x = points[i][0];
        int y = points[i][1];
        int color = gdImageGetPixel(img, x, y);
        
        if (gdImageTrueColor(img)) {
            int r = gdImageRed(img, color);
            int g = gdImageGreen(img, color);
            int b = gdImageBlue(img, color);
            printf("  (%d,%d): RGB(%d,%d,%d) = color index %d\n", 
                   x, y, r, g, b, color);
        } else {
            int r = gdImageRed(img, color);
            int g = gdImageGreen(img, color);
            int b = gdImageBlue(img, color);
            printf("  (%d,%d): palette index %d = RGB(%d,%d,%d)\n", 
                   x, y, color, r, g, b);
        }
    }
    
    gdImageDestroy(img);
    return 0;
}