#include "grid_detection.h"
#include <stdlib.h>

int main()
{
	int grid_w, grid_h;
	int h_lines[200], v_lines[200];
    	int h_count = 0, v_count = 0;

    	unsigned char* grid_img = detect_grid("data/images/level1_pretreated_bw.png",
		       	&grid_w, &grid_h,
			"data/images/detected_grid.png");
    	
	
	if (grid_img)
	{
        	process_grid(grid_img, grid_w, grid_h, 10, 10);
        	free(grid_img);
    	}
    	return 0;
}

