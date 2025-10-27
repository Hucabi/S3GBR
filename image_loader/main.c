#include <stdio.h>
#include <stdlib.h>
#include "image_utils.h"

// --- STB IMPLEMENTATION ---
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
// --------------------------

int main() {
    const char* input_file = "level_1_image_1.jpg";
    const char* output_file = "binarized.png";

    int width, height, channels;
    unsigned char* img = stbi_load(input_file, &width, &height, &channels, 0);
    if (!img) {
        printf("Error: Could not load image %s\n", input_file);
        return 1;
    }

    printf("Loaded image: %dx%d (%d channels)\n", width, height, channels);

    unsigned char* gray = (unsigned char*)malloc(width * height);
    unsigned char* bw_logic = (unsigned char*)malloc(width * height);
    unsigned char* bw_visual = (unsigned char*)malloc(width * height);

    if (!gray || !bw_logic || !bw_visual) {
        printf("Error: Memory allocation failed\n");
        stbi_image_free(img);
        return 1;
    }

    // Preprocessing
    rgb_to_gray(img, gray, width, height, channels);
    binarize(gray, bw_logic, width, height);

    // Convert 0/1 → 0/255 for visualization
    for (int i = 0; i < width * height; i++)
        bw_visual[i] = bw_logic[i] ? 0 : 255;

    stbi_write_png(output_file, width, height, 1, bw_visual, width);
    printf("Saved binarized image as %s\n", output_file);

    // Cleanup
    free(gray);
    free(bw_logic);
    free(bw_visual);
    stbi_image_free(img);

    return 0;
}
