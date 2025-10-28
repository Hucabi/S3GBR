#include <stdio.h>
#include <stdlib.h>
#include "grid_detection.h"
#include "word_list_detection.h"

int main() {
	// Load image
	BinaryImage *binary = load_pretreated_image("level_1_binarized.png");
    	if (!binary) {
        	printf("Error loading image\n");
        	return -1;
    	}

	// Grid detection
	printf("Detecting grid...\n");
    	GridCells grid = detect_grid_from_image(binary);
    	printf("Grid detected: %d x %d (%d cells)\n", grid.rows, grid.cols, grid.count);

	// Word list detection
	printf("Detecting word list...\n");
    	WordList word_list = find_word_list(binary, grid);
    	printf("Word list detected: %d words\n", word_list.count);

	// Visualisation
    	visualize_grid_detection(binary, grid, "grid_detection.png");

	// Saving cells
	save_all_cells_binary(binary, grid, "grid_cell");

	// Free memory (AHAAHAHHAHAHAHAHAH)
	free(grid.rects);
    	word_list_free(&word_list);
    	binary_image_free(binary);

    	return 0;
}
