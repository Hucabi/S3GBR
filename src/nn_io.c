#include <stdio.h>
#include "nn_io.h"

int nn_save(const Network *net, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;

    fwrite(&net->n_in,     sizeof(int), 1, f);
    fwrite(&net->n_hidden, sizeof(int), 1, f);
    fwrite(&net->n_out,    sizeof(int), 1, f);

    fwrite(net->W1, sizeof(double), net->n_in * net->n_hidden, f);
    fwrite(net->b1, sizeof(double), net->n_hidden, f);
    fwrite(net->W2, sizeof(double), net->n_hidden * net->n_out, f);
    fwrite(net->b2, sizeof(double), net->n_out, f);

    fclose(f);
    return 1;
}

static int read_exact(void *ptr, size_t sz, size_t count, FILE *f)
{
    return fread(ptr, sz, count, f) == count;
}


int nn_load(Network *net, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    int ni, nh, no;

    if (!read_exact(&ni, sizeof(int), 1, f) ||
    !read_exact(&nh, sizeof(int), 1, f) ||
    !read_exact(&no, sizeof(int), 1, f)) 
    {
        fclose(f);
        return 0;
    }

    if (ni != net->n_in || nh != net->n_hidden || no != net->n_out) 
    {
        fclose(f);
        return 0;
    }

    if (!read_exact(net->W1, sizeof(double), (size_t)ni * nh, f) ||
        !read_exact(net->b1, sizeof(double), (size_t)nh, f) ||
        !read_exact(net->W2, sizeof(double), (size_t)nh * no, f) ||
        !read_exact(net->b2, sizeof(double), (size_t)no, f)) 
    {
        fclose(f);
        return 0;
    }


        fclose(f);
        return 1;
}
