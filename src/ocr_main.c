#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "nn.h"
#include "dataset.h"
#include "nn_io.h"


int main(int argc, char **argv) 
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s epochs [lr] [seed]\n", argv[0]);
        return 1;
    }

    int epochs = atoi(argv[1]);
    double lr = (argc >= 3) ? atof(argv[2]) : 0.03;

    unsigned seed = (argc >= 4) ? 
        (unsigned)atoi(argv[3]) : (unsigned)time(NULL);


    srand(seed);

    printf("Params: epochs=%d lr=%.4f seed=%u\n", epochs, lr, seed);

    Dataset ds = load_full_dataset("prod_tune", 200);
    if (ds.N == 0) 
    {
        fprintf(stderr, "Dataset vide\n");
        return 1;
    }

    printf("Loaded dataset: %d samples\n", ds.N);

    Network *net = nn_create(1024, 128, 26);
    if (!net) 
        return 1;
    nn_init(net, seed);

    if (nn_load(net, "ocr_weights.bin")) 
    {
        printf("Fine-tuning: loaded existing weights ocr_weights.bin\n");
    } 
    else
    {
        printf("Training from scratch (no existing weights)\n");
    }


    double *y = (double*)malloc(sizeof(double)*26);
    int *perm = (int*)malloc(sizeof(int) * ds.N);

    if (!y || !perm) 
    {
        fprintf(stderr, "alloc y/perm\n");
        free(y); 
        free(perm);
        free_dataset(&ds);
        nn_free(net);
        return 1;
    }

    for (int i=0;i<ds.N;++i)
        perm[i] = i;

    for (int e=1; e<=epochs; ++e) 
    {
        for (int i=ds.N-1;i>0;--i) 
        { 
            int r = rand()%(i+1); 
            int t=perm[i]; 
            perm[i]=perm[r]; 
            perm[r]=t; 
        }

        double loss = 0.0;
        int correct = 0;

        for (int t = 0; t < ds.N; ++t) 
        {
            int i = perm[t];

            one_hot(ds.Y[i], 26, y);
            loss += nn_train_sample_softmax(net, ds.X[i], y, lr);

            nn_forward_softmax(net, ds.X[i]);
            int pred = argmax(net->a2, 26);
            if (pred == ds.Y[i]) 
                ++correct;
        }
        loss /= (double)ds.N;
        double acc = (double)correct / (double)ds.N;

        printf("Epoch %4d  CE=%.4f  Acc=%.2f%%  (lr=%.3f)\n",
             e, loss, acc*100.0, lr);


        if (e % 5 == 0) 
        {
            nn_save(net, "ocr_weights.bin");
        }

    }

    if (!nn_save(net, "ocr_weights.bin")) 
    {
        fprintf(stderr, "Erreur sauvegarde poids\n");
    } 
    else 
    {
        printf("Poids sauvegardés dans ocr_weights.bin\n");
    }

    

    free_dataset(&ds);
    free(y); 
    free(perm);
    nn_free(net);
    return 0;
}