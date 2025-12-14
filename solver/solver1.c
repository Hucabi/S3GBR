#include "solver1.h"
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

int search_err(int dir, int r_start, int c_start, int* dim, char** grid,
	char* word, int len)
{
    int r = r_start;
    int c = c_start;
    int errors = 0;
    const int maxErrors = 2;

    for(int i = 0; i < len; i++)
    {
        if(r < 0 || r >= dim[0] || c < 0 || c >= dim[1])
            return -1;

        if(grid[r][c] != word[i])
            errors++;

        if(errors > maxErrors)
            return -1;

        switch(dir)
        {
            case 0: r--; break;       // up
            case 1: r++; break;       // down
            case 2: c--; break;       // left
            case 3: c++; break;       // right
            case 4: r--; c--; break;  // up-left
            case 5: r--; c++; break;  // up-right
            case 6: r++; c--; break;  // down-left
            case 7: r++; c++; break;  // down-right
        }
    }

    return errors;
}

int* solver(char* file, char* word)
{
    int len = 0;
    for(int i = 0; word[i] != 0; i++) len++;

    char* w = malloc(sizeof(char) * (len + 1));
    strcpy(w, word);
    // convert to upper
    for(int i = 0; i < len; i++)
        if(w[i] >= 'a' && w[i] <= 'z')
            w[i] = (char)(w[i] - 32);

    int dim[2] = {0,0};

    // read grid
    FILE* fp = fopen(file, "r");
    if(fp == NULL) { printf("fopen error\n"); return NULL; }

    dim[0] = count_lines(fp);
    if(dim[0] == -1) { printf("count_lines error\n"); return NULL; }

    if(fseek(fp, 0, SEEK_SET) < 0) { printf("fseek error\n"); return NULL; }

    size_t n = 120;
    char* line = NULL;
    dim[1] = getline(&line, &n, fp);
    char** grid = malloc(sizeof(char*) * dim[0]);
    grid[0] = malloc(sizeof(char) * (dim[1] + 1));
    strcpy(grid[0], line);
    grid[0][strcspn(grid[0], "\r\n")] = '\0';

    for(int i = 1; i < dim[0]; i++)
    {
        getline(&line, &n, fp);
        grid[i] = malloc(sizeof(char) * (dim[1] + 1));
        strcpy(grid[i], line);
        grid[i][strcspn(grid[i], "\r\n")] = '\0';
    }
    fclose(fp);
    free(line);

    int* coords = malloc(sizeof(int) * 4);
    for(int i = 0; i < 4; i++) coords[i] = 0;

    
    int minErrors = 3;
    int bestCoords[4] = {0,0,0,0};

    for(int r = 0; r < dim[0]; r++)
    {
        for(int c = 0; c < dim[1]; c++)
        {
            if(grid[r][c] == w[0])
            {
                for(int dir = 0; dir < 8; dir++)
                {
                    int err = search_err(dir, r, c, dim, grid, w, len);
                    if(err >= 0 && err < minErrors)
                    {
                        minErrors = err;
                        bestCoords[0] = r;
                        bestCoords[1] = c;

                        int dr = 0, dc = 0;
                        switch(dir)
                        {
                            case 0: dr = -1; break;
                            case 1: dr = 1; break;
                            case 2: dc = -1; break;
                            case 3: dc = 1; break;
                            case 4: dr = -1; dc = -1; break;
                            case 5: dr = -1; dc = 1; break;
                            case 6: dr = 1; dc = -1; break;
                            case 7: dr = 1; dc = 1; break;
                        }
                        bestCoords[2] = r + dr * (len - 1);
                        bestCoords[3] = c + dc * (len - 1);
                    }
                }
            }
        }
    }

    for(int i = 0; i < 4; i++)
        coords[i] = bestCoords[i];

    if(minErrors > 2)
        printf("Not found\n");
    else
        printf("(%i,%i)(%i,%i)\n",
               coords[1], coords[0], coords[3], coords[2]);

    
    for(int i = 0; i < dim[0]; i++) free(grid[i]);
    free(grid);
    free(w);

    return coords;
}


int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("incorrect input");
		return 0;
	}

	int dimWord = 0; // nb of words

    FILE* fpWord = fopen(argv[2], "r");
	if(fpWord == NULL) printf("fopen error");
	
	dimWord = count_lines(fpWord);
	if(dimWord == -1) printf("count_lines error");

	if(fseek(fpWord, 0, SEEK_SET) < 0) printf("fseek error");

	size_t nWord = 120;
	char* lineWord = NULL;
	char** listWord = malloc(sizeof(char*)*dimWord);
	for(int i = 0; i < dimWord; i++)
	{
		getline(&lineWord, &nWord, fpWord);
		listWord[i] = malloc(strlen(lineWord) + 1);
		listWord[i] = strcpy(listWord[i], lineWord);
		listWord[i][strcspn(listWord[i], "\r\n")] = '\0';
	}
	fclose(fpWord);

	int** coords = malloc(sizeof(int)*4*dimWord);

	for(int i = 0; i < dimWord; i++)
	{
		coords[i] = solver(argv[1], listWord[i]);
	}

	FILE *coordsFile = fopen("coords.txt", "w");
    if (coordsFile == NULL) printf("coordsFile error");

    for (int i = 0; i < dimWord; i++) {
        fprintf(coordsFile, "%d,%d,%d,%d\n",
            coords[i][1], coords[i][0], coords[i][3], coords[i][2]);
    }

    fclose(coordsFile);

    for(int i = 0; i < dimWord; i++)
            free(coords[i]);
        free(coords);

        return 0;
}
