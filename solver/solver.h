#ifndef SOLVER_H
#define SOLVER_H


void solver(char* file, char* word);

char* uppercase(char* word);

short[] direction(size_t r, size_t c, size_t dim_r, size_t dim_c, size_t len);

size_t[] parkour(short[] dir, char** grid, char* word);

#endif
