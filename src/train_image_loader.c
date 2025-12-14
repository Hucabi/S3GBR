#include <stdio.h>
#include <stdlib.h>
#include "train_image_loader.h"
#include "image_loader.h"
#include "image_utils.h"

#define TW 32
#define TH 32

// Resize nearest-neighbor pour image grayscale
static void resize_nn_u8(const unsigned char *in, int w, int h,
                         unsigned char *out, int tw, int th)
{
    for (int y = 0; y < th; ++y) {
        int sy = (int)((long long)y * h / th);
        for (int x = 0; x < tw; ++x) {
            int sx = (int)((long long)x * w / tw);
            out[y * tw + x] = in[sy * w + sx];
        }
    }
}

int load_image_for_training(const char *path, double **out_vec)
{
    if (!out_vec) return 0;
    *out_vec = NULL;

    // 1) Charger image
    Image img = LoadImage(path);
    if (!img.data || img.width <= 0 || img.height <= 0) {
        fprintf(stderr, "Load failed: %s\n", path);
        return 0;
    }

    int w = img.width;
    int h = img.height;
    int c = img.channels;

    // 2) RGB -> Gray (0..255)
    unsigned char *gray = (unsigned char*)malloc(w * h);
    if (!gray) {
        FreeImage(img);
        return 0;
    }
    rgb_to_gray(img.data, gray, w, h, c);

    // 3) Mettre en 32x32 si besoin
    unsigned char gray32[TW * TH];
    if (w == TW && h == TH) {
        for (int i = 0; i < TW * TH; ++i)
            gray32[i] = gray[i];
    } else {
        resize_nn_u8(gray, w, h, gray32, TW, TH);
    }

    // 4) Conversion directe en vecteur [0..1]
    // 1 = noir (encre), 0 = blanc (fond)
    double *vec = (double*)malloc(sizeof(double) * TW * TH);
    if (!vec) {
        free(gray);
        FreeImage(img);
        return 0;
    }

    for (int i = 0; i < TW * TH; ++i) {
        vec[i] = (255.0 - (double)gray32[i]) / 255.0;
    }


    *out_vec = vec;

    free(gray);
    FreeImage(img);
    return 1;
}
