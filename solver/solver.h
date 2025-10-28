#ifndef SOLVER_H
#define SOLVER_H


void solver(char* file, char* word);

char* uppercase(char* word, int* len);

char** grid_retrieve(char* file, int** dim);

short* direction(int r, int c, int** dim, int len);

short search(int dir, int** coords, int** dim, char** grid, char* word, int len);

short parkour(int** coords, int** dim, char** grid, char* word, int len);

#endif
