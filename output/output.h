#ifndef SOLVER_H
#define SOLVER_H


void load_grid(const char *filename);

void load_words(const char *filename);

void draw_letter(unsigned char *image, int width, int x0, int y0, char ch, int SCALE);

void write_png(const char *filename,unsigned char *image,int width,int height);

int runOutput(void);

#endif
