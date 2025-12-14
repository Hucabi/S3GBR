#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>

#include "ocr_prod.h"
#include "nn.h"
#include "nn_io.h"
#include "train_image_loader.h"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif


static const char *WEIGHTS_PATH = "src/ocr_weights.bin";

static const char *GRID_DIR     = "data/grid/cells";
static const char *WORDS_ROOT   = "data/wordlist/cells";

static const char *OUT_GRID_TXT = "grid.txt";
static const char *OUT_WORD_TXT = "words.txt";


typedef struct 
{
    int id;
    char path[MAX_PATH];
} CellEntry;

static int cmp_cell_id(const void *a, const void *b) 
{
    const CellEntry *x = (const CellEntry*)a;
    const CellEntry *y = (const CellEntry*)b;
    return x->id - y->id;
}

static char predict_char_thresh(Network *net, const char *img_path)
{
    double *x = NULL;
    if (!load_image_for_training(img_path, &x))
        return 'A';

    nn_forward_softmax(net, x);

    int best = 0;
    for (int k = 1; k < 26; ++k)
        if (net->a2[k] > net->a2[best])
            best = k;

    free(x);
    return 'A' + best;
}

static char predict_char_grid(Network *net, const char *img_path)
{
    return predict_char_thresh(net, img_path);
}

static char predict_char_word(Network *net, const char *img_path)
{
    return predict_char_thresh(net, img_path);
}




static CellEntry* scan_grid_cells(const char *dirpath, int *out_n) 
{
    *out_n = 0;
    DIR *d = opendir(dirpath);
    if (!d) 
    { 
        perror(dirpath);
        return NULL; 
    }

    int cap = 256;
    CellEntry *arr = (CellEntry*)malloc(sizeof(CellEntry) * cap);
    if (!arr) 
    { 
        closedir(d); 
        return NULL; 
    }

    struct dirent *de;
    while ((de = readdir(d)) != NULL) 
    {
        if (de->d_name[0] == '.') 
            continue;

        int id;
        if (sscanf(de->d_name, "cell_%d.png", &id) == 1) 
        {
            if (*out_n >= cap) 
            {
                cap *= 2;
                CellEntry *tmp = 
                    (CellEntry*)realloc(arr, sizeof(CellEntry) * cap);
                if (!tmp) 
                { 
                    free(arr); 
                    closedir(d); 
                    return NULL; 
                }
                arr = tmp;
            }
            arr[*out_n].id = id;
            snprintf(arr[*out_n].path, MAX_PATH, "%s/%s", 
                dirpath, de->d_name);
            (*out_n)++;
        }
    }
    closedir(d);

    if (*out_n == 0) 
    { 
        free(arr); 
        return NULL; 
    }

    qsort(arr, *out_n, sizeof(CellEntry), cmp_cell_id);
    return arr;
}

static int write_grid_txt(Network *net) 
{
    int n = 0;
    CellEntry *cells = scan_grid_cells(GRID_DIR, &n);
    if (!cells) 
    {
        fprintf(stderr, "No grid cells found in %s\n", GRID_DIR);
        return 0;
    }

    int side = (int)(sqrt((double)n) + 0.5);
    if (side * side != n) 
    {
        fprintf(stderr, 
            "Grid cells count (%d) is not a perfect square.\n", n);
        fprintf(stderr, "Cannot infer grid size automatically.\n");
        free(cells);
        return 0;
    }

    char *grid = (char*)malloc((size_t)side * side);
    if (!grid) 
    { 
        free(cells); 
        return 0; 
    }
    for (int i = 0; i < side * side; ++i) grid[i] = '?';

    for (int i = 0; i < n; ++i) 
    {
        int id = cells[i].id;
        if (id < 0 || id >= side * side) 
            continue;
        grid[id] = predict_char_grid(net, cells[i].path);
    }

    FILE *f = fopen(OUT_GRID_TXT, "w");
    if (!f) 
    { 
        perror(OUT_GRID_TXT); 
        free(grid); 
        free(cells); 
        return 0; 
    }

    for (int r = 0; r < side; ++r) 
    {
        for (int c = 0; c < side; ++c) 
        {
            fputc(grid[r * side + c], f);
        }
        fputc('\n', f);
    }

    fclose(f);
    free(grid);
    free(cells);
    return 1;
}

static int file_exists(const char *path) 
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int dir_exists(const char *path) 
{
    DIR *d = opendir(path);
    if (!d) return 0;
    closedir(d);
    return 1;
}

static int write_words_txt(Network *net) 
{
    FILE *out = fopen(OUT_WORD_TXT, "w");
    if (!out) 
    { 
        perror(OUT_WORD_TXT);
        return 0; 
    }

    for (int w = 0; ; ++w) 
    {
        char wdir[MAX_PATH];
        snprintf(wdir, sizeof(wdir), "%s/word_%d", WORDS_ROOT, w);

        if (!dir_exists(wdir)) 
        {
            break;
        }

        for (int k = 0; ; ++k) 
        {
            char imgpath[MAX_PATH];
            int n = snprintf(imgpath, sizeof(imgpath),
                 "%s/letter_%d.png", wdir, k);
            if (n < 0 || n >= (int)sizeof(imgpath)) 
            {
                fprintf(stderr, "Path too long, skipping letter %d in %s\n",
                     k, wdir);
                break;
            }


            if (!file_exists(imgpath))
            {
                break;
            }

            char ch = predict_char_word(net, imgpath);
            fputc(ch, out);
        }

        fputc('\n', out);
    }

    fclose(out);
    return 1;
}

int ocr_prod_run(void) 
{
    Network *net = nn_create(1024, 128, 26);
    if (!net) return 1;

    nn_init(net, 0);

    if (!nn_load(net, WEIGHTS_PATH)) 
    {
        fprintf(stderr, "Cannot load weights: %s\n", WEIGHTS_PATH);
        nn_free(net);
        return 1;
    }

    if (!write_grid_txt(net)) 
    {
        fprintf(stderr, "Failed to write grid\n");
        nn_free(net);
        return 1;
    }

    if (!write_words_txt(net)) 
    {
        fprintf(stderr, "Failed to write wordlist\n");
        nn_free(net);
        return 1;
    }

    printf("Wrote %s and %s\n", OUT_GRID_TXT, OUT_WORD_TXT);
    nn_free(net);
    return 0;
}
