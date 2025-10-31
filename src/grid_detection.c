#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "grid_detection.h"
#include "stb_image.h"
#include "stb_image_write.h"

// Create binary image structure
BinaryImage* binary_image_create(int width, int height)
{
    	BinaryImage *img = malloc(sizeof(BinaryImage));
    	img->width = width;
    	img->height = height;
    	img->data = malloc(width * height);
    	return img;
}

void binary_image_free(BinaryImage *img)
{
    	if (img)
	{
        	free(img->data);
        	free(img);
    	}
}

unsigned char* binary_pixel_at(BinaryImage *img, int x, int y) {
    return &img->data[y * img->width + x];}

// Load pre-treated binary image
BinaryImage* load_pretreated_image(const char *filename)
{
    	int width, height, channels;
    	unsigned char *data = stbi_load(filename, &width, &height,
			&channels, 0);
    
    	if (!data) {
        	printf("Error loading image: %s\n", filename);
        	return NULL;
    	}
    
    	BinaryImage *binary = binary_image_create(width, height);
    
    	// Convert to binary (assuming white background, black grid/letters)
    	for (int i = 0; i < width * height; i++) {
        	// Simple threshold - adjust based on your pre-treatment
        	int pixel_value = (channels == 1) ? data[i] : 
                	(0.299 * data[i*channels] + 0.587 * data[i*channels+1]
			 + 0.114 * data[i*channels+2]);
        	binary->data[i] = (pixel_value > 128) ? 255 : 0;
    	}
    
    	stbi_image_free(data);
    	return binary;
}

// Projection profile analysis for grid detection
void compute_projections(BinaryImage *img, int **h_proj, int **v_proj)
{
    	*h_proj = calloc(img->height, sizeof(int));
    	*v_proj = calloc(img->width, sizeof(int));
    
    	// Horizontal projection (sum of black pixels per row)
    	for (int y = 0; y < img->height; y++) {
        	for (int x = 0; x < img->width; x++) {
            	if (*binary_pixel_at(img, x, y) == 0) { // Black pixel
                	(*h_proj)[y]++;
            		}
        	}
    	}
    
    	// Vertical projection (sum of black pixels per column)
    	for (int x = 0; x < img->width; x++) {
        	for (int y = 0; y < img->height; y++) {
            		if (*binary_pixel_at(img, x, y) == 0) { // Black pixel
                		(*v_proj)[x]++;
            		}
        	}
    	}
}

// Find peaks in projection profiles (grid lines)
int* find_peaks(int *projection, int length, int *peak_count,
		double threshold_ratio)
{
    	// Calculate threshold
    	int max_val = 0;
    	for (int i = 0; i < length; i++) {
        	if (projection[i] > max_val) max_val = projection[i];
    	}
    	int threshold = (int)(max_val * threshold_ratio);
    
    	// Count peaks
    	*peak_count = 0;
    	for (int i = 0; i < length; i++) {
        	if (projection[i] > threshold) (*peak_count)++;
    	}
    
    	// Store peak positions
    	int *peaks = malloc(*peak_count * sizeof(int));
    	int idx = 0;
    	for (int i = 0; i < length; i++) {
        	if (projection[i] > threshold) {
            		peaks[idx++] = i;
        		}
    	}
    
    	return peaks;
}

// Group adjacent peaks to handle thick grid lines
int* group_peaks(int *peaks, int peak_count, int max_gap, int *group_count)
{
    	if (peak_count == 0) {
        	*group_count = 0;
        	return NULL;
    	}
    
    	int *groups = malloc(peak_count * sizeof(int));
    	int group_idx = 0;
    
    	int current_start = peaks[0];
    	int current_end = peaks[0];
    
    	for (int i = 1; i < peak_count; i++) {
        	if (peaks[i] - current_end <= max_gap) {
            		current_end = peaks[i];
        	}
		else
		{
            		// End current group, start new one
            		groups[group_idx++] =
				(current_start + current_end) / 2; // midpoint
            		current_start = peaks[i];
            		current_end = peaks[i];
        	}
    	}
    
    	// Add last group
    	groups[group_idx++] = (current_start + current_end) / 2;
    
    	*group_count = group_idx;
    	return groups;
}

// Main grid localization function
GridCells detect_grid_from_image(BinaryImage *binary) {
    	GridCells cells = {0, 0, 0, NULL}; // rows, cols, count, rects
    
    	// Compute projection profiles
    	int *h_proj, *v_proj;
    	compute_projections(binary, &h_proj, &v_proj);
    
    	// Find peaks (grid lines)
    	int h_peak_count, v_peak_count;
    	int *h_peaks = find_peaks(h_proj, binary->height, &h_peak_count, 0.44);
    	int *v_peaks = find_peaks(v_proj, binary->width, &v_peak_count, 0.44);
    
    	printf("Detected %d horizontal, %d vertical line candidates\n",
			h_peak_count, v_peak_count);
    
    	// Group adjacent peaks
    	int h_group_count, v_group_count;
    	int *h_groups = group_peaks(h_peaks, h_peak_count, 5, &h_group_count);
    	int *v_groups = group_peaks(v_peaks, v_peak_count, 5, &v_group_count);
    
    	printf("Grouped to %d horizontal, %d vertical grid lines\n",
			h_group_count, v_group_count);
    
    	// Calculate grid dimensions: lines-1 = cells
    	cells.rows = h_group_count - 1;
    	cells.cols = v_group_count - 1;
    	cells.count = cells.rows * cells.cols;
    
    	if (cells.count <= 0) {
        	printf("ERROR: Invalid grid dimensions %dx%d\n",
				cells.rows, cells.cols);
        	goto cleanup;
    	}
    
    	printf("Detected grid size: %d rows x %d columns\n",
			cells.rows, cells.cols);
    
    	// Verify minimum size from project specs
    	if (cells.rows < 5 || cells.cols < 5) {
        	printf("WARNING: Grid size %dx%d",cells.rows, cells.cols);
		printf("is below minimum 5x5 requirement\n");	
    	}
    
    	// Create cells from grid lines
    	cells.rects = malloc(cells.count * sizeof(Rectangle));
    
    	int cell_idx = 0;
    	for (int row = 0; row < cells.rows; row++) {
        	for (int col = 0; col < cells.cols; col++) {
            		int x_start = v_groups[col] + 2;
            		int x_end = v_groups[col + 1] - 2;
            		int y_start = h_groups[row] + 2;
            		int y_end = h_groups[row + 1] - 2;
            
            		// Ensure valid dimensions
            		if (x_end > x_start && y_end > y_start) {
                		cells.rects[cell_idx].x = x_start;
                		cells.rects[cell_idx].y = y_start;
                		cells.rects[cell_idx].w = x_end - x_start;
                		cells.rects[cell_idx].h = y_end - y_start;
                		cell_idx++;
            		}
        	}
    	}
    
    	// Update actual count in case some cells were invalid
    	cells.count = cell_idx;

	cleanup:
    		free(h_proj); free(v_proj);
    		free(h_peaks); free(v_peaks);
    		free(h_groups); free(v_groups);
    
   	return cells;
}

// Alternative method: contour-based cell detection
GridCells localize_cells_contours(BinaryImage *binary, int expected_size)
{
    	GridCells cells = {0, 0, 0, NULL}; // rows, cols, count, rects
    
    	// This is a simplified version because OpenCV where
    	// But for C we use basic approach
    
    	cells.count = expected_size * expected_size;
    	cells.rects = malloc(cells.count * sizeof(Rectangle));
    
    	// Estimate cell size based on image dimensions and expected grid size
    	int approx_cell_width = binary->width / expected_size;
    	int approx_cell_height = binary->height / expected_size;
    
    	printf("Estimated cell size: %dx%d\n",
			approx_cell_width, approx_cell_height);
    
    	// Create grid based on estimated dimensions
    	int cell_idx = 0;
    	for (int row = 0; row < expected_size; row++) {
        	for (int col = 0; col < expected_size; col++) {
            		int x = col * approx_cell_width + 2;  // Padding
            		int y = row * approx_cell_height + 2;
            		int w = approx_cell_width - 4; // Avoid borders
            		int h = approx_cell_height - 4;
            
            		// Ensure we stay within image bounds
            		if (x + w > binary->width) w = binary->width - x;
            		if (y + h > binary->height) h = binary->height - y;
            
            		cells.rects[cell_idx].x = x;
            		cells.rects[cell_idx].y = y;
            		cells.rects[cell_idx].w = w;
            		cells.rects[cell_idx].h = h;
            
            		cell_idx++;
        	}
    	}
    
    	return cells;
}

// Extract cell content for Tim OCR
BinaryImage* resize_cell(BinaryImage *cell,
		int target_width, int target_height)
{
    	BinaryImage *resized =
		binary_image_create(target_width, target_height);
    	// Simple nearest-neighbor resizing implementation
    	for (int y = 0; y < target_height; y++) {
        	for (int x = 0; x < target_width; x++) {
            		int src_x = (x * cell->width) / target_width;
            		int src_y = (y * cell->height) / target_height;
            		resized->data[y * target_width + x] =
				*binary_pixel_at(cell, src_x, src_y);
        	}
    	}
    return resized;
}

BinaryImage* extract_cell_content(BinaryImage *original, Rectangle cell)
{
   	if (cell.w <= 0 || cell.h <= 0) return NULL;
    
    	BinaryImage *cell_img = binary_image_create(cell.w, cell.h);
    
    	for (int y = 0; y < cell.h; y++) {
        	for (int x = 0; x < cell.w; x++) {
            		int orig_x = cell.x + x;
            		int orig_y = cell.y + y;
            
            		if (orig_x < original->width && orig_y
					< original->height) {
                		cell_img->data[y * cell.w + x] =
					*binary_pixel_at(original,
						orig_x, orig_y);
            		}
			else
			{
                		cell_img->data[y * cell.w + x] = 255; // White
            		}
        	}
    	}
   	BinaryImage *resized = resize_cell(cell_img, 28, 28);
	binary_image_free(cell_img);	
    	return resized;
}

// Save binary image as PNG
void save_binary_image(BinaryImage *img, const char *filename)
{
    	// Convert binary to RGB for saving
    	unsigned char *rgb_data = malloc(img->width * img->height * 3);
    
    	for (int i = 0; i < img->width * img->height; i++) {
        	unsigned char val = img->data[i];
        	rgb_data[i*3] = val;
        	rgb_data[i*3+1] = val;
        	rgb_data[i*3+2] = val;
    	}
    
    	stbi_write_png(filename, img->width, img->height, 3, rgb_data, 0);
    	free(rgb_data);
}

// Visualize detected cells on original image
void visualize_grid_detection(BinaryImage *original, GridCells cells,
		const char *output_path)
{
	// Create RGB version for visualization
    	unsigned char *rgb_data = malloc(original->width *
			original->height * 3);
    
    	// Convert binary to grayscale RGB
    	for (int i = 0; i < original->width * original->height; i++) {
        	unsigned char val = original->data[i];
        	rgb_data[i*3] = val;
        	rgb_data[i*3+1] = val;
        	rgb_data[i*3+2] = val;
    	}
    
    	// Draw red rectangles for each cell
    	for (int i = 0; i < cells.count; i++) {
        	Rectangle r = cells.rects[i];
        
        // Draw top and bottom borders
        for (int x = r.x; x < r.x + r.w && x < original->width; x++) {
            if (r.y < original->height) {
                int idx = (r.y * original->width + x) * 3;
                rgb_data[idx] = 255;   // Red
                rgb_data[idx+1] = 0;
                rgb_data[idx+2] = 0;
            }
            if (r.y + r.h < original->height) {
                int idx = ((r.y + r.h) * original->width + x) * 3;
                rgb_data[idx] = 255;
                rgb_data[idx+1] = 0;
                rgb_data[idx+2] = 0;
            }
        }
        
        // Draw left and right borders
        for (int y = r.y; y < r.y + r.h && y < original->height; y++) {
            if (r.x < original->width) {
                int idx = (y * original->width + r.x) * 3;
                rgb_data[idx] = 255;
                rgb_data[idx+1] = 0;
                rgb_data[idx+2] = 0;
            }
            if (r.x + r.w < original->width) {
                int idx = (y * original->width + (r.x + r.w)) * 3;
                rgb_data[idx] = 255;
                rgb_data[idx+1] = 0;
                rgb_data[idx+2] = 0;
            }
        }
    }
    stbi_write_png(output_path, original->width, original->height,
		    3, rgb_data, 0);
    free(rgb_data);
}

// Save all cells as binary PNG files
int save_all_cells_binary(BinaryImage *original, GridCells cells,
		const char *base_path) {
    	int saved_count = 0;

    	for (int i = 0; i < cells.count; i++) {
        	BinaryImage *cell_content = extract_cell_content(original,
				cells.rects[i]);
        	if (cell_content) {
            		char filename[256];
            		snprintf(filename, sizeof(filename),
					"%s/cell_%03d.png", base_path, i);
            		save_binary_image(cell_content, filename);
            		binary_image_free(cell_content);
            		saved_count++;
        	}
    	}

    	return saved_count;
}

void grid_cells_free(GridCells *cells) {
    if (cells && cells->rects) {
        free(cells->rects);
        cells->rects = NULL;
        cells->count = 0;
        cells->rows = 0;
        cells->cols = 0;
    }
}
