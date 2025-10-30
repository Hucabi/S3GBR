#ifndef GRID_DETECTION_H
#define GRID_DETECTION_H

typedef struct {
    	int x, y, w, h;
} Rectangle;

typedef struct {
    	int rows, cols;
	int count;
    	Rectangle *rects;
} GridCells;

typedef struct {
    	int width, height;
    	unsigned char *data;  // 0 = black, 255 = white
} BinaryImage;

// Function declaration
BinaryImage* binary_image_create(int width, int height);
void binary_image_free(BinaryImage *img);

BinaryImage* load_pretreated_image(const char *filename);
void save_binary_image(BinaryImage *img, const char *filename);

void compute_projections(BinaryImage *img, int **h_proj, int **v_proj);
int* find_peaks(int *projection, int length, int *peak_count, double threshold_ratio);
int* group_peaks(int *peaks, int peak_count, int max_gap, int *group_count);
GridCells detect_grid_from_image(BinaryImage *binary);

GridCells localize_cells_contours(BinaryImage *binary, int expected_size);

BinaryImage* extract_cell_content(BinaryImage *original, Rectangle cell);
BinaryImage* resize_cell(BinaryImage *cell, int target_width, int target_height);

void visualize_grid_detection(BinaryImage *original, GridCells cells, const char *output_path);
int save_all_cells_binary(BinaryImage *original, GridCells cells, const char *base_path);

void grid_cells_free(GridCells *cells);
#endif
