#include <stdio.h>
#include <stdlib.h>
#include "image_utils.h"


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"



void rotate_90(unsigned char* src, unsigned char* dest, int width, int height) {
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            dest[x * height + (height - 1 - y)] = src[y * width + x];
}


void rotate_180(unsigned char* src, unsigned char* dest, int width, int height) {
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            dest[(height - 1 - y) * width + (width - 1 - x)] = src[y * width + x];
}


void rotate_270(unsigned char* src, unsigned char* dest, int width, int height) {
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            dest[(width - 1 - x) * height + y] = src[y * width + x];
}


int main() {
    	

	//creating files to stock image and loading 
	const char* input_file = "level_1_image_1.jpg";
    	const char* output_file = "binarized.png";
    	const char* rot_file    = "rotated.png";

    	int width, height, channels;
    	unsigned char* img = stbi_load(input_file, &width, &height, &channels, 0);
    	if (!img) 
	{
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
	
    
    // rotation
    int angle;
    printf("Enter rotation angle (90, 180, 270): ");
    scanf("%d", &angle);

    unsigned char* rotated = malloc(width * height);
    int new_width = width, new_height = height;

    switch(angle) {
        case 90:
            rotate_90(bw_visual, rotated, width, height);
            new_width = height; new_height = width;
            break;
        case 180:
            rotate_180(bw_visual, rotated, width, height);
            break;
        case 270:
           rotate_270(bw_visual, rotated, width, height);
            new_width = height; new_height = width;
            break;
        default:
            printf("⚠️ Invalid angle. Skipping rotation.\n");
            rotated = bw_visual; 
            break;
    	}

    	stbi_write_png(rot_file, new_width, new_height, 1, rotated, new_width);
    	printf("Rotated image saved as %s\n", rot_file);


    	// Cleanup
    	free(gray);
    	free(bw_logic);
    	free(bw_visual);
    	stbi_image_free(img);
    	if (rotated != bw_visual) free(rotated);

    	return 0;
}
