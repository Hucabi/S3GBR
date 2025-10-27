#include "grid_detection.h"
#include <stdlib.h>

int main() {
    	int grid_w, grid_h;
    	int *h_lines = NULL, *v_lines = NULL;
    	int h_count = 0, v_count = 0;

    	unsigned char* grid_img = detect_grid(
        	"data/images/level1_pretreated_bw.png",
        	&grid_w, &grid_h,
        	"data/images/detected_grid.png",
        	&h_lines, &h_count,
        	&v_lines, &v_count);

    	if (grid_img) {
        	process_grid(grid_img, grid_w, grid_h, h_lines, h_count, v_lines, v_count);
        	free(grid_img);
    	}

    	free(h_lines);
    	free(v_lines);

    	return 0;
}
