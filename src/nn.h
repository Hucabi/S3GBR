#pragma once

#include <stddef.h>

// ============================================================================
// Structures principales
// ============================================================================

typedef struct {
    int n_in;        // nombre de neurones d'entrée
    int n_hidden;    // nombre de neurones dans la couche cachée
    int n_out;       // nombre de neurones de sortie

    // Poids et biais
    double *W1;      // Poids entre entrée -> couche cachée (taille n_hidden * n_in)
    double *b1;      // Biais de la couche cachée (taille n_hidden)
    double *W2;      // Poids entre couche cachée -> sortie (taille n_out * n_hidden)
    double *b2;      // Biais de la couche de sortie (taille n_out)

    // Buffers pour le calcul avant (forward)
    double *z1;      // Sommes pondérées de la couche cachée
    double *a1;      // Sorties de la couche cachée
    double *z2;      // Sommes pondérées de la couche de sortie
    double *a2;      // Sorties finales du réseau
} Network;

// ============================================================================
// Fonctions d'activation
// ============================================================================

double sigmoid(double x);

double dsigmoid_from_output(double y);

void softmax(const double *z, int n, double *out);

double cross_entropy_from_onehot(const double *p, const double *y, int n);

int argmax(const double *p, int n);

void one_hot(int cls, int n_out, double *y);

// ============================================================================
// Fonctions de gestion du réseau
// ============================================================================

Network* nn_create(int n_in, int n_hidden, int n_out);

void nn_free(Network *net);

void nn_init(Network *net, unsigned seed);

// ============================================================================
// Fonctions d'apprentissage et de prédiction
// ============================================================================

void nn_forward(Network *net, const double *x);

double nn_train_sample(Network *net, const double *x, 
    const double *y, double lr);

void nn_predict(Network *net, const double *x, double *yhat);

void nn_forward_softmax(Network *net, const double *x);

double nn_train_sample_softmax(Network *net,
    const double *x, const double *y, double lr);