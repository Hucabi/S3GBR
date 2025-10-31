#ifndef SOLVER_H
#define SOLVER_H


void solver(char* file, char* word);

short search(int dir, int* coords, int* dim, char** grid, char* word, int len);

#endif
