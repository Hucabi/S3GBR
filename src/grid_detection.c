#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


// Detect line positions from projection
int detect_lines(const int* proj, int size, int** lines, double threshold)
{
    *lines = malloc(size * sizeof(int));
    int count = 0;
    for (int i = 1; i < size - 1; i++) {
        if (proj[i] > threshold && proj[i - 1] <= threshold) {
            (*lines)[count++] = i;
        }
    }
    return count;
}


// Detect grid and return cropped image
unsigned char* detect_grid(const char* image_path, int* grid_w, int* grid_h,
		const char* save_path,
		int** h_lines, int* h_count,
                int** v_lines, int* v_count)
{
    	int width, height, channels;
    	unsigned char* img = stbi_load(image_path, &width, &height, &channels, 1);
    	if (!img)
	{
       		printf("Error: Cannot load image %s\n", image_path);
        	return NULL;
    	}

	int* h_proj = calloc(height, sizeof(int));
    	int* v_proj = calloc(width, sizeof(int));
	
	// Compute projections
    	for (int y = 0; y < height; y++) {
        	for (int x = 0; x < width; x++) {
            	if (img[y * width + x] < 50) {
                	h_proj[y]++;
                	v_proj[x]++;
            		}
        	}
    	}

    	// Compute mean and stddev for horizontal
    	double h_sum = 0, h_sq_sum = 0;
    	for (int y = 0; y < height; y++) {
        	h_sum += h_proj[y];
        	h_sq_sum += h_proj[y] * h_proj[y];
    	}
    	double h_mean = h_sum / height;
    	double h_std = sqrt((h_sq_sum / height) - (h_mean * h_mean));

    	// Compute mean and stddev for vertical
    	double v_sum = 0, v_sq_sum = 0;
    	for (int x = 0; x < width; x++) {
       		v_sum += v_proj[x];
        	v_sq_sum += v_proj[x] * v_proj[x];
    	}
    	double v_mean = v_sum / width;
    	double v_std = sqrt((v_sq_sum / width) - (v_mean * v_mean));

    	// Find crop boundaries using peaks
    	double h_threshold = h_mean + h_std * 1.2; // 20% above average
    	double v_threshold = v_mean + v_std * 1.5; // 50% above average

	*h_count = detect_lines(h_proj, height, h_lines, h_threshold);
	*v_count = detect_lines(v_proj, width, v_lines, v_threshold);

	int top = (*h_count > 0) ? (*h_lines)[0] : -1;
    	int bottom = (*h_count > 0) ? (*h_lines)[*h_count - 1] : -1;
    	int left = (*v_count > 0) ? (*v_lines)[0] : -1;
    	int right = (*v_count > 0) ? (*v_lines)[*v_count - 1] : -1;

    	if (top == -1 || bottom == -1 || left == -1 || right == -1) {
        	printf("Error: Could not detect grid.\n");
        	free(h_proj);
        	free(v_proj);
        	stbi_image_free(img);
        	return NULL;
    	}

    	*grid_w = right - left + 1;
    	*grid_h = bottom - top + 1;

    	printf("Detected grid: top=%d bottom=%d left=%d right=%d\n", top, bottom, left, right);


	// Crop grid
    	unsigned char* grid_img = malloc((*grid_w) * (*grid_h));
    	for (int y = 0; y < *grid_h; y++)
	{
        	for (int x = 0; x < *grid_w; x++)
		{
           		grid_img[y * (*grid_w) + x] = img[(top + y) * width + (left + x)];
        	}
    	}

    	if (save_path)
	{
        	stbi_write_png(save_path, *grid_w, *grid_h, 1, grid_img, *grid_w);
        	printf("Grid image saved to %s\n", save_path);
    	}

	free(h_proj);
	free(v_proj);
    	stbi_image_free(img);
    	return grid_img;
}

void process_grid(unsigned char* grid_img, int grid_w, int grid_h,
		int* h_lines, int h_count, int* v_lines, int v_count)
{
    	int border = 2;

	printf("Processing grid: %d horizontal lines, %d vertical lines\n", h_count, v_count);	

	
	for (int i = 0; i < h_count - 1; i++) {
        	for (int j = 0; j < v_count - 1; j++) {
            		int cell_x = v_lines[j] + border;
            		int cell_y = h_lines[i] + border;
            		int cell_w = (v_lines[j + 1] - v_lines[j]) - 2 * border;
            		int cell_h = (h_lines[i + 1] - h_lines[i]) - 2 * border;
			// Validate cell dimensions
            		if (cell_w <= 0 || cell_h <= 0) continue;
            		if (cell_x < 0 || cell_y < 0) continue;
            		if (cell_x + cell_w > grid_w || cell_y + cell_h > grid_h) continue;

            		// Allocate and copy cell pixels
            		unsigned char* cell = malloc(cell_w * cell_h);
            		for (int y = 0; y < cell_h; y++) {
                		for (int x = 0; x < cell_w; x++) {
                    			cell[y * cell_w + x] = grid_img[(cell_y + y) * grid_w + (cell_x + x)];
                		}			
            		}
	

            	char filename[100];
            	sprintf(filename, "data/cells/cell_%d_%d.png", i, j);
            	stbi_write_png(filename, cell_w, cell_h, 1, cell, cell_w);
            	free(cell);
        }
    }
}
