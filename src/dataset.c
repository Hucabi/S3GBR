#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dataset.h"
#include "train_image_loader.h"

Dataset load_full_dataset(const char *root_dir, int max_per_class)
{
    Dataset ds = {0};

    int capacity = 26 * max_per_class;
    ds.X = (double**)malloc(sizeof(double*) * capacity);
    ds.Y = (int*)malloc(sizeof(int) * capacity);
    ds.N = 0;

    if (!ds.X || !ds.Y) {
        fprintf(stderr, "Dataset allocation failed\n");
        return ds;
    }

    for (char letter = 'a'; letter <= 'z'; ++letter) {
        char dir[256];
        snprintf(dir, sizeof(dir), "%s/%c", root_dir, letter);

        for (int i = 0; i < max_per_class; ++i) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%d.png", dir, i);

            double *vec = NULL;
            if (!load_image_for_training(path, &vec)) {
                continue; // image absente ou invalide
            }

            ds.X[ds.N] = vec;
            ds.Y[ds.N] = letter - 'a';
            ds.N++;
        }
    }

    printf("Dataset loaded: %d samples\n", ds.N);
    return ds;
}

void free_dataset(Dataset *ds)
{
    if (!ds || !ds->X || !ds->Y) return;

    for (int i = 0; i < ds->N; ++i)
        free(ds->X[i]);

    free(ds->X);
    free(ds->Y);

    ds->X = NULL;
    ds->Y = NULL;
    ds->N = 0;
}
