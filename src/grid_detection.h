#ifndef GRID_DETECTION_H
#define GRID_DETECTION_H

unsigned char* detect_grid(const char* image_path, int* grid_w, int* grid_h, const char* save_path);
void process_grid(unsigned char* grid_img, int grid_w, int grid_h, int rows, int cols);

#endif
