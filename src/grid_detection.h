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

// Image I/O
BinaryImage* load_pretreated_image(const char *filename);
void save_binary_image(BinaryImage *img, const char *filename);
void binary_image_free(BinaryImage *img);

// Grid localization
GridCells detect_grid_from_image(BinaryImage *binary);
GridCells localize_cells_contours(BinaryImage *binary, int expected_size);

// Cell extraction
BinaryImage* extract_cell_content(BinaryImage *original, Rectangle cell);
int save_all_cells_binary(BinaryImage *original, GridCells cells, const char *base_path);

// Visualization
void visualize_grid_detection(BinaryImage *original, GridCells cells, const char *output_path);

#endif
