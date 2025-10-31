#include <stdio.h>
#include "image_utils.h"
#include "stb_image.h"
#include "stb_image_write.h"


//turns the pixel to gray
void rgb_to_gray(unsigned char* img, unsigned char* gray, int width, int height, 
		int channels) {
    for (int i = 0; i < width * height; i++) {
        int idx = i * channels;
        unsigned char r = img[idx + 0];
        unsigned char g = (channels > 1) ? img[idx + 1] : r;
        unsigned char b = (channels > 2) ? img[idx + 2] : r;
        gray[i] = (unsigned char)(0.2126 * r + 0.7152 * g + 0.0722 * b + 0.5);
    }
}





void compute_histogram(unsigned char* gray, int width, 
		int height, int hist[256]) {
    for (int i = 0; i < 256; i++) hist[i] = 0;
    for (int i = 0; i < width * height; i++) hist[gray[i]]++;
}




//using otsu_threshold to see if we turn the pixel to black or white

int otsu_threshold(unsigned char* gray, int width, int height) {
    int hist[256];
    compute_histogram(gray, width, height, hist);

    long total = width * height;
    long sum = 0;
    for (int t = 0; t < 256; t++) sum += t * hist[t];

    long sumB = 0;
    long wB = 0;
    long wF = 0;
    double maxVar = 0.0;
    int threshold = 0;

    for (int t = 0; t < 256; t++) {
        wB += hist[t];
        if (wB == 0) continue;
        wF = total - wB;
        if (wF == 0) break;

        sumB += t * hist[t];
        double mB = (double)sumB / wB;
        double mF = (double)(sum - sumB) / wF;

        double varBetween = (double)wB * wF * (mB - mF) * (mB - mF);
        if (varBetween > maxVar) {
            maxVar = varBetween;
            threshold = t;
        }
    }
    return threshold;
}

//binarizes an image
void binarize(unsigned char* gray, unsigned char* bw, int width, int height) {
    int threshold = otsu_threshold(gray, width, height);

    for (int i = 0; i < width * height; i++) {
        bw[i] = (gray[i] <= threshold) ? 1 : 0;
    }
}

