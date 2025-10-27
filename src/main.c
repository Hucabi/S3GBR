#include <stdio.h>
#include <stdlib.h>
#include "grid_detection.h"

int main() {
    	const char *input_path = "data/images/level_1_binarized.png";
    	const char *output_viz_path = "data/images/detected_grid.png";
    	const char *cells_output_dir = "data/cells/cell";
    
    	printf("=== Word Search Grid Cell Localization ===\n");
    	printf("Input: %s\n", input_path);
    
    	// Load the pre-treated binary image
    	BinaryImage *binary = load_pretreated_image(input_path);
    	if (!binary) {
        	printf("ERROR: Failed to load input image\n");
        	return 1;
    	}
    
    	printf("Loaded binary image: %dx%d pixels\n", binary->width, binary->height);
    
    	// Localize grid cells using projection method
    	GridCells cells = detect_grid_from_image(binary);
    
    	if (cells.count > 0) {
        	printf("SUCCESS: Detected %dx%d grid with %d total cells\n", 
               	cells.rows, cells.cols, cells.count);
        
        	// Save visualization
        	visualize_grid_detection(binary, cells, output_viz_path);
        	printf("Saved visualization: %s\n", output_viz_path);
        
        	// Extract and save all cells as binary images
        	printf("Extracting cells to: %s*.png\n", cells_output_dir);
        	int saved_count = save_all_cells_binary(binary, cells, cells_output_dir);
        	printf("Saved %d binary cell images\n", saved_count);
        
        	free(cells.rects);
    	}
	else
	{
        	printf("ERROR: Failed to detect grid cells\n");
    	}
    
    	binary_image_free(binary);
    	printf("=== Program finished ===\n");
    	return 0;
}
