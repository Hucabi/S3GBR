#include "nn.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

static double rand_uniform(double a, double b) 
{
    // Fonction qui renvoi un double compris entre a et b (-1 et 1 ici)
    return a + (b - a) * ((double)rand() / (double)RAND_MAX);
}

double sigmoid(double x) 
{
    // Fonction d'activation
    if (x < -15.0) x = -15.0;
    if (x >  15.0) x =  15.0;
    return 1.0 / (1.0 + exp(-x));
}

double dsigmoid_from_output(double y) 
{
    // Dérivée de la fonction d'activation pour rétro-propagation
    return y * (1.0 - y);
}

Network* nn_create(int n_in, int n_hidden, int n_out) 
{
    // Fonction de creation du réseau de neurones

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

    // Allocation mémoire poids et biais
    net->W1 = (double*)malloc(sizeof(double) * n_hidden * n_in);
    net->b1 = (double*)calloc(n_hidden, sizeof(double));
    net->W2 = (double*)malloc(sizeof(double) * n_out * n_hidden);
    net->b2 = (double*)calloc(n_out, sizeof(double));

    // Allocation mémoire buffer forward
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
    // Fonction pour free tout le réseau en cas d'erreur

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
    // Fonction d'initialisation du réseau de neurones
    if (!net) return;
    srand(seed);

    // Initialisation avec des valeurs proches de 0 pour rester stable

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
    /* Fonction de calcul des valeurs des neuronnes de la couche cachée
    et de sortie par somme des w1x1 + b1 et w2x2 + b2 
    ainsi que la fonction d'activation*/

    if (!net || !x) return;

    // Couche cachée : z1 = W1 * x + b1 ; a1 = sigmoid(z1)
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

    // Sortie : z2 = W2 * a1 + b2 ; a2 = sigmoid(z2) (binaire)
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

double nn_train_sample(Network *net, const double *x, 
    const double *y, double lr) 
{
    // Fonction d'entrainement du réseau de neurones
    if (!net || !x || !y) return 0.0;

    // Fonction forward
    nn_forward(net, x);

    // delta de sortie (différence valeur obtenue et valeur attendue)
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

    //  delta couche cachée ()
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

    //  mise à jour W2, b2
    for (int k = 0; k < net->n_out; ++k) 
    {
        double *W2_row = &net->W2[k * net->n_hidden];
        for (int j = 0; j < net->n_hidden; ++j) 
        {
            W2_row[j] -= lr * delta2[k] * net->a1[j];
        }
        net->b2[k] -= lr * delta2[k];
    }

    // mise à jour W1, b1
    for (int j = 0; j < net->n_hidden; ++j) 
    {
        double *W1_row = &net->W1[j * net->n_in];
        for (int i = 0; i < net->n_in; ++i) 
        {
            W1_row[i] -= lr * delta1[j] * x[i];
        }
        net->b1[j] -= lr * delta1[j];
    }

    /*
    MSE (erreur quadratique moyenne) :
    fonction de perte (compare la valeur de sortie avec la valeur cible)
    */ 
    double mse = 0.0;
    for (int k = 0; k < net->n_out; ++k) 
    {
        double diff = net->a2[k] - y[k];
        mse += diff * diff;
    }

    free(delta1);
    free(delta2);
    return mse;
}

void nn_predict(Network *net, const double *x, double *yhat)
{
    // Fonction de prediction
    if (!net || !x || !yhat) return;
    nn_forward(net, x);
    for (int k = 0; k < net->n_out; ++k) 
    {
        yhat[k] = net->a2[k];
    }
}