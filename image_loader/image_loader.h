#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

typedef struct {
    unsigned char* data;
    int width;
    int height;
    int channels;
} Image;
Image LoadImage(const char* path);
void FreeImage(Image img);
#endif
