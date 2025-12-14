#include "nn.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

static double rand_uniform(double a, double b) 
{
    return a + (b - a) * ((double)rand() / (double)RAND_MAX);
}

double sigmoid(double x) 
{
    if (x < -15.0) x = -15.0;
    if (x >  15.0) x =  15.0;
    return 1.0 / (1.0 + exp(-x));
}

double dsigmoid_from_output(double y) 
{
    return y * (1.0 - y);
}

void softmax(const double *z, int n, double *out) 
{
    double m = z[0];
    for (int i = 1; i < n; ++i)
    {
        if (z[i] > m) m = z[i];
    } 

    double s = 0.0;

    for (int i = 0; i < n; ++i) 
    { 
        out[i] = exp(z[i] - m); s += out[i]; 
    }

    for (int i = 0; i < n; ++i)
    {
        out[i] /= (s > 0.0 ? s : 1.0);
    }
}

double cross_entropy_from_onehot(const double *p, const double *y, int n) 
{
    const double eps = 1e-12;
    double L = 0.0;
    for (int k = 0; k < n; ++k) 
    {
        if (y[k] > 0.5) 
        { 
            L = -log(p[k] + eps); 
            break; 
        }
    }
    return L;
}

int argmax(const double *p, int n)
{
    if (!p || n <= 0) return -1;

    int best = 0;
    for (int i = 1; i < n; ++i) 
    {
        if (p[i] > p[best]) best = i;
    }
    return best;
}


void one_hot(int cls, int n_out, double *y) 
{
    for (int k = 0; k < n_out; ++k)
    {
        y[k] = 0.0;
    }
    if (cls >= 0 && cls < n_out)
    {
        y[cls] = 1.0;
    }
}

Network* nn_create(int n_in, int n_hidden, int n_out) 
{
    if (n_in <= 0 || n_hidden <= 0 || n_out <= 0) 
    {
        fprintf(stderr, "[nn_create] dimensions invalides\n");
        return NULL;
    }

    Network *net = (Network*)calloc(1, sizeof(Network));

    if (!net) 
    {
        fprintf(stderr, "[nn_create] échec d'allocation Network\n");
        return NULL;
    }

    net->n_in = n_in;
    net->n_hidden = n_hidden;
    net->n_out = n_out;

    /* Poids & biais */
    net->W1 = (double*)malloc(sizeof(double) * n_hidden * n_in);
    net->b1 = (double*)calloc(n_hidden, sizeof(double));
    net->W2 = (double*)malloc(sizeof(double) * n_out * n_hidden);
    net->b2 = (double*)calloc(n_out, sizeof(double));

    /* Buffers forward */
    net->z1 = (double*)malloc(sizeof(double) * n_hidden);
    net->a1 = (double*)malloc(sizeof(double) * n_hidden);
    net->z2 = (double*)malloc(sizeof(double) * n_out);
    net->a2 = (double*)malloc(sizeof(double) * n_out);

    if (!net->W1 || !net->b1 || !net->W2 || !net->b2 ||
        !net->z1 || !net->a1 || !net->z2 || !net->a2) 
    {
        fprintf(stderr, "[nn_create] échec d'allocation des buffers\n");
        nn_free(net);
        return NULL;
    }

    return net;
}

void nn_free(Network *net) 
{
    if (!net) return;
    free(net->W1);
    free(net->b1);
    free(net->W2);
    free(net->b2);
    free(net->z1);
    free(net->a1);
    free(net->z2);
    free(net->a2);
    free(net);
}

void nn_init(Network *net, unsigned seed) 
{
    if (!net) return;
    srand(seed);

    /* Init de type "Xavier-ish" simple pour petites tailles */
    double scale1 = 1.0 / sqrt((double)net->n_in);
    double scale2 = 1.0 / sqrt((double)net->n_hidden);

    for (int j = 0; j < net->n_hidden; ++j) 
    {
        for (int i = 0; i < net->n_in; ++i) 
        {
            net->W1[j * net->n_in + i] = rand_uniform(-1.0, 1.0) * scale1;
        }
        net->b1[j] = 0.0;
    }

    for (int k = 0; k < net->n_out; ++k) 
    {
        for (int j = 0; j < net->n_hidden; ++j) 
        {
            net->W2[k * net->n_hidden + j] = rand_uniform(-1.0, 1.0) * scale2;
        }
        net->b2[k] = 0.0;
    }
}

void nn_forward(Network *net, const double *x) 
{
    if (!net || !x) return;

    /* Couche cachée : z1 = W1 * x + b1 ; a1 = sigmoid(z1) */
    for (int j = 0; j < net->n_hidden; ++j) 
    {
        double sum = net->b1[j];
        const double *W1_row = &net->W1[j * net->n_in];
        for (int i = 0; i < net->n_in; ++i) 
        {
            sum += W1_row[i] * x[i];
        }
        net->z1[j] = sum;
        net->a1[j] = sigmoid(sum);
    }

    /* Sortie : z2 = W2 * a1 + b2 ; a2 = sigmoid(z2) (binaire) */
    for (int k = 0; k < net->n_out; ++k) 
    {
        double sum = net->b2[k];
        const double *W2_row = &net->W2[k * net->n_hidden];
        for (int j = 0; j < net->n_hidden; ++j) 
        {
            sum += W2_row[j] * net->a1[j];
        }
        net->z2[k] = sum;
        net->a2[k] = sigmoid(sum);
    }
}

void nn_forward_softmax(Network *net, const double *x) 
{
    if (!net || !x) 
        return;

    // cachée: sigmoïde (inchangé)
    for (int j = 0; j < net->n_hidden; ++j) 
    {
        double sum = net->b1[j];
        const double *W1_row = &net->W1[j * net->n_in];
        for (int i = 0; i < net->n_in; ++i)
        {
            sum += W1_row[i] * x[i];
        }

        net->z1[j] = sum;
        net->a1[j] = sigmoid(sum);
    }

    // sortie: logits puis softmax
    for (int k = 0; k < net->n_out; ++k) 
    {
        double sum = net->b2[k];
        const double *W2_row = &net->W2[k * net->n_hidden];
        for (int j = 0; j < net->n_hidden; ++j)
        {
            sum += W2_row[j] * net->a1[j];
        }

        net->z2[k] = sum;
    }
    softmax(net->z2, net->n_out, net->a2);
}

double nn_train_sample(Network *net,
     const double *x, const double *y, double lr)
{
    if (!net || !x || !y) return 0.0;

    /* 1) forward */
    nn_forward(net, x);

    /* 2) delta de sortie */
    double *delta2 = (double*)malloc(sizeof(double) * net->n_out);
    if (!delta2) 
    {
        fprintf(stderr, "[nn_train_sample] alloc delta2\n");
        return 0.0;
    }
    for (int k = 0; k < net->n_out; ++k) 
    {
        double err = net->a2[k] - y[k];
        delta2[k] = err * dsigmoid_from_output(net->a2[k]);
    }

    /* 3) delta couche cachée */
    double *delta1 = (double*)malloc(sizeof(double) * net->n_hidden);
    if (!delta1) 
    {
        free(delta2);
        fprintf(stderr, "[nn_train_sample] alloc delta1\n");
        return 0.0;
    }
    for (int j = 0; j < net->n_hidden; ++j) 
    {
        double sum = 0.0;
        for (int k = 0; k < net->n_out; ++k) 
        {
            sum += net->W2[k * net->n_hidden + j] * delta2[k];
        }
        delta1[j] = sum * dsigmoid_from_output(net->a1[j]);
    }

    /* 4) mise à jour W2, b2 */
    for (int k = 0; k < net->n_out; ++k) 
    {
        double *W2_row = &net->W2[k * net->n_hidden];
        for (int j = 0; j < net->n_hidden; ++j) 
        {
            W2_row[j] -= lr * delta2[k] * net->a1[j];
        }
        net->b2[k] -= lr * delta2[k];
    }

    /* 5) mise à jour W1, b1 */
    for (int j = 0; j < net->n_hidden; ++j) 
    {
        double *W1_row = &net->W1[j * net->n_in];
        for (int i = 0; i < net->n_in; ++i) 
        {
            W1_row[i] -= lr * delta1[j] * x[i];
        }
        net->b1[j] -= lr * delta1[j];
    }

    /* 6) MSE du sample (utile pour logging) */
    double mse = 0.0;
    for (int k = 0; k < net->n_out; ++k) 
    {
        double diff = net->a2[k] - y[k];
        mse += diff * diff;
    }

    free(delta1);
    free(delta2);
    return mse; /* pour XOR, n_out=1 donc c'est la MSE univariée */
}

double nn_train_sample_softmax(Network *net, 
    const double *x, const double *y, double lr) 
{
    if (!net || !x || !y) 
        return 0.0;

    nn_forward_softmax(net, x);

    // delta2 = p - y
    double *delta2 = (double*)malloc(sizeof(double) * net->n_out);
    if (!delta2) 
        return 0.0;
    for (int k = 0; k < net->n_out; ++k)
    {
        delta2[k] = net->a2[k] - y[k];
    }

    // delta1 = (W2^T * delta2) ⊙ sig'(a1)
    double *delta1 = (double*)malloc(sizeof(double) * net->n_hidden);
    if (!delta1) 
    { 
        free(delta2); 
        return 0.0; 
    }
    for (int j = 0; j < net->n_hidden; ++j) 
    {
        double sum = 0.0;
        for (int k = 0; k < net->n_out; ++k)
        {
            sum += net->W2[k * net->n_hidden + j] * delta2[k];
        }    

        delta1[j] = sum * dsigmoid_from_output(net->a1[j]);
    }

    // update W2, b2
    for (int k = 0; k < net->n_out; ++k) 
    {
        double *W2_row = &net->W2[k * net->n_hidden];

        for (int j = 0; j < net->n_hidden; ++j)
        {
            W2_row[j] -= lr * delta2[k] * net->a1[j];
        }

        net->b2[k] -= lr * delta2[k];
    }

    // update W1, b1
    for (int j = 0; j < net->n_hidden; ++j) 
    {
        double *W1_row = &net->W1[j * net->n_in];

        for (int i = 0; i < net->n_in; ++i)
        {
            W1_row[i] -= lr * delta1[j] * x[i];
        }

        net->b1[j] -= lr * delta1[j];
    }

    double ce = cross_entropy_from_onehot(net->a2, y, net->n_out);
    free(delta1); free(delta2);
    return ce;
}


void nn_predict(Network *net, const double *x, double *yhat) 
{
    if (!net || !x || !yhat) return;
    nn_forward(net, x);
    for (int k = 0; k < net->n_out; ++k) 
    {
        yhat[k] = net->a2[k];
    }
}