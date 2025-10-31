#include "solver.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int count_lines(FILE* file)
{
	char buf[65536];
	int counter = 0;
	for(;;)
	{
		size_t res = fread(buf,1,65536,file);
		if(ferror(file)) return -1;
		size_t i;
		for(i = 0; i < res; i++)
			if(buf[i] == '\n')
				counter++;
		if(feof(file))
			break;
	}
	return counter;
}

void solver(char* file, char* word)
{
	int len = 0; // len of word
	
	for(int i = 0; word[i] != 0; i++) len++;

	char* w = malloc(sizeof(char)*len+1);
	w = strcpy(w, word);
	// uppercase
	for(int i = 0; i < len; i++)
	{
		if(word[i] >= 'a' && word[i] <= 'z')
			w[i] = (char)(word[i] - 32);
	}


	int dim[2] = {0,0}; // 0 = r, 1 = c
	dim[0] = 0;
	dim[1] = 0;


	// grid retrieve
	
	FILE* fp = fopen(file, "r");
	if(fp == NULL) printf("fopen error");
	
	dim[0] = count_lines(fp);
	if(dim[0] == -1) printf("count_lines error");

	if(fseek(fp, 0, SEEK_SET) < 0) printf("fseek error");

	size_t n = 120;
	char* line = NULL;
	dim[1] = getline(&line, &n , fp);
	char** grid = malloc(sizeof(char)*dim[0]*dim[1]);
	grid[0] = malloc(sizeof(char)*dim[1]+1);
	grid[0] = strcpy(grid[0], line);
	for(int i = 1; i < dim[0]; i++)
	{
		getline(&line, &n, fp);
		grid[i] = malloc(sizeof(char)*dim[1]+1);
		grid[i] = strcpy(grid[i], line);
	}
	fclose(fp);
	
	int coords[4] = {0};
	for(int i = 0; i < 4; i++) coords[i] = 0;
	// 0,1 = row,col index
	// 2,3 = final row,col index
	
	// go through grid
	short b = 0;
	while(!b && coords[0] < dim[0] && coords[1] < dim[1])
	{
		if(grid[coords[0]][coords[1]] == w[0])
		{
			// [U,D,L,R]
			short dir[4] = {0,0,0,0};
			if(coords[0]+1>=len) dir[0] = 1;
			if(dim[0]>=len+coords[0]) dir[1] = 1;
			if(coords[1]+1>=len) dir[2] = 1;
			if(dim[1]>=len+coords[1]) dir[3] = 1;
			if(dir[0]) // up
			{
				if(dir[2])
					b=search(4,coords,dim,grid,w,len);
				if(!b && dir[3])
					b=search(5,coords,dim,grid,w,len);
				if(!b)
					b=search(0,coords,dim,grid,w,len);
			}
			if(!b && dir[1]) // down
			{
				if(dir[2])
					b=search(6,coords,dim,grid,w,len);
				if(!b && dir[3])
					b=search(7,coords,dim,grid,w,len);
				if(!b)
					b=search(1,coords,dim,grid,w,len);
			}
			if(!b && dir[2]) // left
				b=search(2,coords,dim,grid,w,len);
			if(!b && dir[3]) // right
				b=search(3,coords,dim,grid,w,len);
			
			// SIGNATURE SEARCH
		}
		if(!b)
		{
			coords[1]++;
			if(coords[1] == dim[1])
			{
				coords[1] = 0;
				coords[0]++;
			}
			coords[2] = coords[0];
			coords[3] = coords[1];
		}
	}
	if(!b) printf("Not found\n");
	else printf("(%i,%i)(%i,%i)\n",
			coords[1], coords[0], coords[3], coords[2]);
	
	// free
	free(line);
	free(w);
	for(int i = 0; i < dim[0]; i++)
		free(grid[i]);
	free(grid);
}


short search(int dir, int* coords, int* dim, char** grid, char* word, int len)
{
	// dir => {0:up, down, left, right, uL, uR, dL, 7:dR}
	
	short valid = 1;
	
	int r = coords[0];
	int c = coords[1];
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
			while(valid && r < dim[0] && i < len)
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
			while(valid && i < len && c < dim[1])
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
			while(valid && i < len && r > 0 && c < dim[1])
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
			while(valid && i < len && r < dim[0] && c > 0)
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
			while(valid && i < len && r < dim[0] && c < dim[1])
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
		coords[2] = r;
		coords[3] = c;
	}

	return valid;
}


int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("incorrect input");
		return 0;
	}

	solver(argv[1], argv[2]);
	return 0;
}
