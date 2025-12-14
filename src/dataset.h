#ifndef DATASET_H
#define DATASET_H

typedef struct {
    double **X;   // images : N x 1024
    int    *Y;    // labels : N
    int     N;    // nombre d'échantillons
} Dataset;


Dataset load_full_dataset(const char *root_dir, int max_per_class);

void free_dataset(Dataset *ds);

#endif
