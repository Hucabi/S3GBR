#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void detect_and_split_grid(const char *image_path, int rows, int cols)
{
	int width, height, channels;
	unsigned char *img = stbi_load(image_path, &width, &height, &channels, 1);
	if (!img)
	{
        	printf("Error: Cannot load image %s\n", image_path);
        	return;
	}
	
	
	// Allocate projection arrays
	int *h_proj = calloc(height, sizeof(int));
	int *v_proj = calloc(width, sizeof(int));

	
	for (int y = 0; y < height; y++)
	{
        	for (int x = 0; x < width; x++)
		{
			unsigned char pixel = img[y * width + x];
            		if (pixel == 0) // black pixe
			{
                		h_proj[y]++;
                		v_proj[x]++;
            		}
        	}
    	}
	
	// Enhanced projections
    	for (int i = 1; i < width - 1; i++)
		v_proj[i] = (v_proj[i - 1] + v_proj[i] + v_proj[i + 1]) / 3;
    	for (int i = 1; i < height - 1; i++)
		h_proj[i] = (h_proj[i - 1] + h_proj[i] + h_proj[i + 1]) / 3;


	// Compute averages
	int h_sum = 0, v_sum = 0;
	for (int y = 0; y < height; y++) h_sum += h_proj[y];
	for (int x = 0; x < width; x++) v_sum += v_proj[x];


	int h_avg = h_sum / height;
	int v_avg = v_sum / width;

	int h_lines[200], v_lines[200];
    	int h_count = 0, v_count = 0;

	// Detection	
	int min_spacing = 20; // minimum distance between lines
	int max_lines = 50;   // safety limit

	for (int y = 0; y < height; y++)
	{
    		if (h_proj[y] > h_avg * 2)
		{
        		if ((h_count == 0 || abs(h_lines[h_count - 1] - y) > min_spacing) && h_count < max_lines)
            			h_lines[h_count++] = y;
    		}
	}

	for (int x = 0; x < width; x++)
	{
    		if (v_proj[x] > v_avg * 1.2)
		{
        		if ((v_count == 0 || abs(v_lines[v_count - 1] - x) > min_spacing) && v_count < max_lines)
            			v_lines[v_count++] = x;
    		}
	}

	printf("Filtered: %d horizontal and %d vertical lines\n", h_count, v_count);

	// Fallback if detection fails
	if (h_count < 2 || v_count < 2)
	{
    		printf("Fallback to uniform split: %d rows x %d cols\n", rows, cols);
    		h_count = rows + 1;
    		v_count = cols + 1;
    		for (int i = 0; i < h_count; i++) h_lines[i] = i * (height / rows);
    		for (int j = 0; j < v_count; j++) v_lines[j] = j * (width / cols);
	}

	
	// Crop cells
    	for (int i = 0; i < h_count - 1; i++)
	{
        	for (int j = 0; j < v_count - 1; j++)
		{
            		int cell_w = v_lines[j + 1] - v_lines[j];
            		int cell_h = h_lines[i + 1] - h_lines[i];
            		unsigned char *cell = malloc(cell_w * cell_h);
            		for (int y = 0; y < cell_h; y++)
			{
                		for (int x = 0; x < cell_w; x++)
				{
                    			cell[y * cell_w + x] = img[(h_lines[i] + y) * width + (v_lines[j] + x)];
				}
			}
			char filename[100];
            		sprintf(filename, "data/cells/cell_%d_%d.png", i, j);
            		stbi_write_png(filename, cell_w, cell_h, 1, cell, cell_w);
            		free(cell);
		}
	}
	free(h_proj);
	free(v_proj);
	stbi_image_free(img);
}
