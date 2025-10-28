#include "solver.h"
#include <stdio.h>
#include <stdlib.h>

void solver(char* file, char* word)
{
	int len = 0; // len of word
	char* w = uppercase(word, &len);
	int* dim[2] = {0,0}; // 0 = r, 1 = c
	*dim[0] = 0;
	*dim[1] = 0;
	char** grid = grid_retrieve(file, dim);

	int* coords[4] = {0};
	for(int i = 0; i < 4; i++) *coords[i] = 0;
	// 0,1 = row,col index
	// 2,3 = final row,col index
	
	// go through grid
	short found = 0;
	while(!found && coords[0] < dim[0] && coords[1] < dim[1])
	{
		if(grid[*coords[0]][*coords[1]] == w[0])
		{
			found = parkour(coords, dim, grid, w, len);
		}
		if(!found)
		{
			coords[1]++;
			if(*coords[1] == *dim[1])
			{
				coords[1] = 0;
				coords[0]++;
			}
			*coords[2] = *coords[0];
			*coords[3] = *coords[1];
		}
	}
	if(!found) printf("Not found\n");
	else printf("(%i,%i)(%i,%i)\n", *coords[0], *coords[1], *coords[2], *coords[3]);
}
/*
char* uppercase(char* word, int* len)
{
	char* res = word;
	for(int i = 0; res[i] != 0; i++)
	{
		*len = *len + 1;
		if(res[i] >= 'a' && res[i] <= 'z')
			res[i] = (char)(res[i] - 32);
	}
	return res;
}
*/

char* uppercase(char* w, int* len)
{
	for(int i = 0; w[i] != 0; i++)
	{
		*len = *len + 1;
	}
	char res[*len];
	for(int i = 0; i < *len; i++)
	{
		if(w[i] >= 'a' && w[i] <= 'z')
			res[i] = (char)(w[i] - 32);
	}

	return res;
}

int count_lines(FILE* file)
{
	char buf[65536];
	int counter = 0;
	for(;;)
	{
		size_t res = fread(buf, 1, 65536, file);
		if (ferror(file))
			return -1;
		size_t i;
		for(i = 0; i < res; i++)
			if (buf[i] == '\n')
				counter++;
		if (feof(file))
			break;
	}
	return counter;
}

char** grid_retrieve(char* file, int** dim)
{
	FILE* f = fopen(file, "r");

	if (f == NULL)
		errx(EXIT_FAILURE, "file open error");

	*dim[0] = count_lines(f);
	if(*dim[0] == -1)
		errx(EXIT_FAILURE, "count_lines error");

	if(fseek(f, 0, SEEK_SET) < 0)
		errx(EXIT_FAILURE, "fseek error");

	char** line[*dim[0]];
	size_t* n = malloc(sizeof(size_t));
	*n = 120;
	*dim[1] = getline(line[0], n, f);
	for(int i = 1; i < *dim[0]; i++)
	{
		getline(line[i], n, f);
	}
	free(n);
	fclose(f);
	return *line;
}

short* direction(int r, int c, int** dim, int len)
{
	// [Up, Down, Left, Right]
	short* res[4] = {0};
	if(r < len) *res[0] = 1;
	if(*dim[0] < len + r) *res[1] = 1;
	if(c < len) *res[2] = 1;
	if(*dim[1] < len+c) *res[3] = 1;
	return *res;
}

short search(int dir, int** coords, int** dim, char** grid, char* word, int len)
{
	// dir => {0:up, down, left, right, uL, uR, dL, 7:dR}
	
	short valid = 1;
	
	int r = *coords[0];
	int c = *coords[1];
	int i = 0;

	switch(dir)
	{
		case 0:
			while(valid && r > 0 && i < len)
			{
				valid = valid && grid[r][c] == word[i];
				i++;
				if(i != len && valid)
					r--;
			}
			break;
		case 1:
			while(valid && r < *dim[0] && i < len)
			{
				valid = valid && grid[r][c] == word[i];
				i++;
				if(i != len && valid)
					r++;
			}
			break;
		case 2:
			while(valid && i < len && c > 0)
			{
				valid = valid && grid[r][c] == word[i];
				i++;
				if(i != len && valid)
					c--;
			}
			break;
		case 3:
			while(valid && i < len && c < *dim[1])
			{
				valid = valid && grid[r][c] == word[i];
				i++;
				if(i != len && valid)
					c++;
			}
			break;
		case 4:
			while(valid && i < len && r > 0 && c > 0)
			{
				valid = valid && grid[r][c] == word[i];
				i++;
				if(i != len && valid)
				{
					r--;
					c--;
				}
			}
			break;
		case 5:
			while(valid && i < len && r > 0 && c < *dim[1])
			{
				valid = valid && grid[r][c] == word[i];
				i++;
				if(i != len && valid)
				{
					r--;
					c++;
				}
			}
			break;
		case 6:
			while(valid && i < len && r < *dim[0] && c > 0)
			{
				valid = valid && grid[r][c] == word[i];
				i++;
				if(i != len && valid)
				{
					r++;
					c--;
				}
			}
			break;
		case 7:
			while(valid && i < len && r < *dim[0] && c < *dim[1])
			{
				valid = valid && grid[r][c] == word[i];
				i++;
				if(i != len && valid)
				{
					r++;
					c++;
				}
			}
			break;
	}

	if(valid)
	{
		*coords[2] = r;
		*coords[3] = c;
	}

	return valid;
}

short parkour(int** coords, int** dim, char** grid, char* word, int len)
// return 1 if found(with coords changed), 0 otherwise
{
	short found = 0;
	short* dir = direction(*coords[0], *coords[1], dim, len);

	if(dir[0]) // up
	{
		if(dir[2])
			found = search(4, coords, dim, grid, word, len);
		if(!found && dir[3])
			found = search(5, coords, dim, grid, word, len);
		if(!found)
			found = search(0, coords, dim, grid, word, len);
	}
	if(!found && dir[1]) // down
	{
		if(dir[2])
			found = search(6, coords, dim, grid, word, len);
		if(!found && dir[3])
			found = search(5, coords, dim, grid, word, len);
		if(!found)
			found = search(1, coords, dim, grid, word, len);
	}
	if(!found && dir[2]) // left
	{
		found = search(2, coords, dim, grid, word, len);
	}
	if(!found && dir[3]) // right
	{
		found = search(3, coords, dim, grid, word, len);
	}

	return found;
}

int main(int argc, char* argv[])
{
//	solver(argv[1], argv[2]);
	solver("grid", "horizontal");	
	return 0;
}
