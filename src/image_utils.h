#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

void rgb_to_gray(unsigned char* img, unsigned char* gray, int width,
	       	int height, int channels);
void compute_histogram(unsigned char* gray, int width,
	       	int height, int hist[256]);
int otsu_threshold(unsigned char* gray, int width,
	       	int height);
void binarize(unsigned char* gray, unsigned char* bw,
	       	int width, int height);

#endif
