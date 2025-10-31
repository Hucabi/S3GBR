#include <stdio.h>
#include <stdlib.h>
#include "nn.h"

static void print_pred(Network *net, double a, double b, double target) 
{
    double x[2] = {a, b};
    double yhat[1];
    nn_predict(net, x, yhat);
    int cls = (yhat[0] >= 0.5) ? 1 : 0;
    printf("Input (%.0f, %.0f) -> pred=%.6f (cls=%d)  target=%.0f\n",
           a, b, yhat[0], cls, target);
}

int main(int argc, char **argv) 
{
    // paramètres modifiables par arguments
    int epochs = 10000;
    double lr = 0.5;
    unsigned seed = (unsigned) 42;

    if (argc >= 2) epochs = atoi(argv[1]);         // ex: ./xor_nn 20000
    if (argc >= 3) lr = atof(argv[2]);             // ex: ./xor_nn 20000 0.3
    if (argc >= 4) seed = (unsigned)atoi(argv[3]); // ex: ./xor_nn 20000 0.3 42

    printf("Params: epochs=%d, lr=%.4f, seed=%u\n", epochs, lr, seed);

    // Réseau 2-2-1 pour XOR
    Network *net = nn_create(2, 2, 1);
    if (!net) {
        fprintf(stderr, "Erreur: nn_create a échoué\n");
        return 1;
    }
    nn_init(net, seed);

    // Dataset XOR
    const double X[4][2] = {
        {0.0, 0.0},
        {0.0, 1.0},
        {1.0, 0.0},
        {1.0, 1.0}
    };
    const double Y[4][1] = {
        {1.0},
        {0.0},
        {0.0},
        {1.0}
    };

    // Entraînement (SGD sur les 4 échantillons)
    for (int e = 1; e <= epochs; ++e) {
        double loss = 0.0;

        // Option simple: itérer toujours dans le même ordre (suffit pour XOR)
        for (int i = 0; i < 4; ++i) {
            loss += nn_train_sample(net, X[i], Y[i], lr);
        }
        loss /= 4.0;

        if (e % 1000 == 0 || e == 1 || e == epochs) {
            printf("Epoch %5d  MSE=%.8f\n", e, loss);
        }
    }

    // Tests finaux
    printf("\n=== Tests finaux ===\n");
    print_pred(net, 0, 0, 1);
    print_pred(net, 0, 1, 0);
    print_pred(net, 1, 0, 0);
    print_pred(net, 1, 1, 1);

    nn_free(net);
    return 0;
}
