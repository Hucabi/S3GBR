#include "grid_detection.h"

int main()
{
	detect_and_split_grid("data/images/level1_pretreated_bw.png", 10, 10);
	return 0;
}
// compile with gcc src/main.c src/grid_detection.c -o grid_split
