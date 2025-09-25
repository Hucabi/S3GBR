#include "solver.h"
#include <stdio.h>

void solver(char* file, char* word)
{
	char* w = uppercase(word);

	char** grid;
	
}

char* uppercase(char* word)
{
	char* res = word;
	for(int i = 0; res[i] == 0; i++)
	{
		if(res[i] >= 'a' && res[i] <= 'z')
			res[i] = res[i] - 32;
	}
}

short[] direction(size_t r, size_t c, size_t dim_r, size_t dim_c, size_t len);

size_t[] parkour(short[] dir, char** grid, char* word);
