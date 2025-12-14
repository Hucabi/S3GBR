#ifndef ROTATION_H
#define ROTATION_H

void preprocess_ocr(unsigned char* img, unsigned char* bw, 
                    int width, int height, int channels);
double find_best_angle(unsigned char* gray, int width, int height);
double row_variance(unsigned char* row, int width);
double compute_variance_score(unsigned char* gray, int width, int height);
void rotate_image(unsigned char* src, unsigned char* dest,
                  int width, int height, double angle_deg);
void save_rotation_test_images(unsigned char* image, int width, int height);

#endif
