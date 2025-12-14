#ifndef NN_IO_H
#define NN_IO_H

#include "nn.h"

int nn_save(const Network *net, const char *path);
int nn_load(Network *net, const char *path);

#endif
