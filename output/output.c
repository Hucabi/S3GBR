#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#define CELL 64
#define MAX_ROWS 100
#define MAX_COLS 100

int ROWS = 0, COLS = 0;
char grid[MAX_ROWS][MAX_COLS];
int highlight[MAX_ROWS][MAX_COLS] = {0};


// 5x5 bitmap
unsigned char font[26][5] = {
    {0x1F,0x05,0x05,0x1F,0x00}, // A
    {0x1F,0x15,0x15,0x0A,0x00}, // B
    {0x0E,0x11,0x11,0x11,0x00}, // C
    {0x1F,0x11,0x11,0x0E,0x00}, // D
    {0x1F,0x15,0x15,0x11,0x00}, // E
    {0x1F,0x05,0x05,0x01,0x00}, // F
    {0x0E,0x11,0x15,0x1D,0x00}, // G
    {0x1F,0x04,0x04,0x1F,0x00}, // H
    {0x11,0x1F,0x11,0x00,0x00}, // I
    {0x08,0x10,0x10,0x0F,0x00}, // J
    {0x1F,0x04,0x0A,0x11,0x00}, // K
    {0x1F,0x10,0x10,0x10,0x00}, // L
    {0x1F,0x02,0x04,0x02,0x1F}, // M
    {0x1F,0x02,0x04,0x1F,0x00}, // N
    {0x0E,0x11,0x11,0x0E,0x00}, // O
    {0x1F,0x05,0x05,0x02,0x00}, // P
    {0x0E,0x11,0x19,0x1E,0x00}, // Q
    {0x1F,0x05,0x0D,0x12,0x00}, // R
    {0x12,0x15,0x15,0x09,0x00}, // S
    {0x01,0x1F,0x01,0x00,0x00}, // T
    {0x0F,0x10,0x10,0x0F,0x00}, // U
    {0x07,0x08,0x10,0x08,0x07}, // V
    {0x1F,0x08,0x04,0x08,0x1F}, // W
    {0x11,0x0A,0x04,0x0A,0x11}, // X
    {0x01,0x02,0x1C,0x02,0x01}, // Y
    {0x19,0x15,0x13,0x00,0x00}  // Z
};

void load_grid(const char *filename){
    FILE *f = fopen(filename,"r");
    if(!f){ perror("grid file"); exit(1); }

    char line[1024];
    int r = 0;
    while(fgets(line,sizeof(line),f)){
        int len = strlen(line);
        if(len>0 && (line[len-1]=='\n' || line[len-1]=='\r')) len--;
        for(int c=0;c<len;c++){
            grid[r][c] = line[c];
        }
        if(COLS < len) COLS = len;
        r++;
    }
    ROWS = r;
    COLS--;
    fclose(f);
}

void load_words(const char *filename){
    FILE *f = fopen(filename,"r");
    if(!f){ perror("words file"); exit(1); }
    int x1,y1,x2,y2;
    while(fscanf(f,"%d,%d,%d,%d",&x1,&y1,&x2,&y2)==4){
        int dx = (x2 > x1) - (x2 < x1);
        int dy = (y2 > y1) - (y2 < y1);
        int x=x1,y=y1;
        while(1){
            highlight[y][x]=1;
            if(x==x2 && y==y2) break;
            x+=dx; y+=dy;
        }
    }
    fclose(f);
}


int SCALE = 4;

void draw_letter(unsigned char *image, int width, int x0, int y0, char ch,
    int SCALE) {
    if(ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
    if(ch<'A'||ch>'Z') return;
    int idx = ch-'A';
    for(int col=0; col<5; col++){
        for(int row=0; row<5; row++){
            if((font[idx][col] >> row) & 1){
                for(int dy=0; dy<SCALE; dy++){
                    for(int dx=0; dx<SCALE; dx++){
                        int x = x0 + col*SCALE + dx;
                        int y = y0 + row*SCALE + dy;
                        int pixel = (y*width + x)*3;
                        image[pixel] = 0;
                        image[pixel+1] = 0;
                        image[pixel+2] = 0;
                    }
                }
            }
        }
    }
}


void write_png(const char *filename,unsigned char *image,int width,int height){
    FILE *fp = fopen(filename,"wb");
    if(!fp){ perror("fopen"); exit(1); }

    png_structp png=png_create_write_struct(PNG_LIBPNG_VER_STRING,
        NULL,NULL,NULL);
    png_infop info=png_create_info_struct(png);
    if(setjmp(png_jmpbuf(png))){ fprintf(stderr,"PNG error\n"); exit(1); }
    png_init_io(png,fp);
    png_set_IHDR(png,info,width,height,8,PNG_COLOR_TYPE_RGB,PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);

    png_bytep *rows = malloc(sizeof(png_bytep)*height);
    for(int y=0;y<height;y++) rows[y]=image+y*width*3;
    png_write_info(png,info);
    png_write_image(png,rows);
    png_write_end(png,NULL);

    free(rows);
    png_destroy_write_struct(&png,&info);
    fclose(fp);
}


int runOutput(void){
    load_grid("grid.txt");
    load_words("coords.txt");

    int width = COLS*CELL;
    int height = ROWS*CELL;
    unsigned char *image = malloc(width*height*3);

    // draw bg and highlights
    for(int y=0;y<height;y++){
        for(int x=0;x<width;x++){
            int row=y/CELL;
            int col=x/CELL;
            unsigned char r,g,b;
            if(highlight[row][col]){
                r=255; g=255; b=120;
            }else{
                r=g=b=255;
            }
            int idx=(y*width+x)*3;
            image[idx]=r; image[idx+1]=g; image[idx+2]=b;
        }
    }

    // draw grid lines
    for(int r=0;r<=ROWS;r++){
        for(int x=0;x<width;x++){
            int y=r*CELL;
            if(y<height){
                int idx=(y*width+x)*3;
                image[idx]=0; image[idx+1]=0; image[idx+2]=0;
            }
        }
    }
    for(int c=0;c<COLS;c++){
        for(int y=0;y<height;y++){
            int x=c*CELL;
            if(x<width){
                int idx=(y*width+x)*3;
                image[idx]=0; image[idx+1]=0; image[idx+2]=0;
            }
        }
    }

    // draw letters
    for(int r=0;r<ROWS;r++){
        for(int c=0;c<COLS;c++){
            int SCALE = 4;
            int x0 = c*CELL + (CELL-5*SCALE)/2;
            int y0 = r*CELL + (CELL-5*SCALE)/2;
            draw_letter(image, width, x0, y0, grid[r][c], SCALE);
        }
    }

    write_png("output.png",image,width,height);
    free(image);

    return 0;
}
