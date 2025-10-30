#include <stdio.h>
#include <stdlib.h>
#include "grid_detection.h"
#include "word_list_detection.h"

int main() {
	printf("Word Search Solver - Starting...\n");
    
    	// Load image
    	BinaryImage *binary = load_pretreated_image("data/images/level_1_binarized.png");
    	if (!binary) {
        	printf("Error loading image at data/images/level_1_binarized.png\n");
        
        	// Créer une image de test si le fichier n'existe pas
        	binary = binary_image_create(100, 100);
        	for (int i = 0; i < 100 * 100; i++) {
            		binary->data[i] = 255; // Image blanche
        	}
    	}

    	printf("Image loaded: %d x %d\n", binary->width, binary->height);

    	// Grid search
    	printf("Detecting grid...\n");
    	GridCells grid = detect_grid_from_image(binary);
    	printf("Grid detected: %d x %d (%d cells)\n", grid.rows, grid.cols, grid.count);
	
    	// Wordlist search
    	printf("Detecting word list...\n");
    	WordList word_list = find_word_list(binary, grid);
    	printf("Word list detected: %d words\n", word_list.count);
	
	// Visualisation of the word list if found
	if (word_list.count > 0) {
    		visualize_word_list(binary, word_list, "data/images/word_list_detection.png");
		save_word_list_cells(binary, word_list, "data");
		save_word_letters(binary, word_list, "data");
	}

    	// Saving cells if grid found
    	if (grid.count > 0) {
        	int saved = save_all_cells_binary(binary, grid, "data/grid/cells");
        	printf("Saved %d grid cells to data/cells/\n", saved);
        
        	// Visualisation
        	visualize_grid_detection(binary, grid, "data/images/grid_detection.png");
        	printf("Grid visualization saved as data/images/grid_detection.png\n");
    	}

    	// Free memory (AHAHAHAHAHAHAHAHAHAHAH)
    	if (grid.rects) free(grid.rects);
    	if (word_list.words) {
        	for (int i = 0; i < word_list.count; i++) {
            		free(word_list.words[i]);
        	}
        	free(word_list.words);
    	}
    	if (word_list.word_rects) free(word_list.word_rects);

    	binary_image_free(binary);
			
   	printf("Program completed successfully.\n");
    	return 0;
}
