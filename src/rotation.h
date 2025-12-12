#ifndef ROTATION.H
#define ROTATION.H



void preprocess_ocr(unsigned char* img,   // RGB input
                    unsigned char* bw,    // output binary
                    int width, int height,
                    int channels);
double find_best_angle(unsigned char* gray, int width, int height);
double row_variance(unsigned char* row, int width);
double compute_variance_score(unsigned char* gray, int width, int height);
