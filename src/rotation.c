#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "image_loader.h"
#include "image_utils.h"
#include "grid_detection.h"
#include "word_list_detection.h"








double row_variance(unsigned char* row, int width)
        {
                double sum = 0.0;
                double sum_sq = 0.0;

                for (int i = 0; i < width; i++)
                {
                        double v = row[i];
                        sum +=v;
                        sum_sq += v*v;
                }
                double mean = sum / width;
                double mean_sq = sum_sq / width;

                return mean_sq - mean*mean;
        }


double compute_variance_score(unsigned char* gray, int width, int height)
{
        double score = 0.0;
        for (int y = 0; y < height; y++)
        {
                unsigned char *row = &gray[y * width];
                score += row_variance(row,width);

        }
        return score;

}

void rotate_image(unsigned char* src,
                  unsigned char* dest,
                  int width,
                  int height,
                  double angle_deg)
{
    double angle = angle_deg * M_PI / 180.0;
    double cosA = cos(angle);
    double sinA = sin(angle);

    int cx = width / 2;
    int cy = height / 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            double xt = x - cx;
            double yt = y - cy;

            double srcX =  xt * cosA + yt * sinA + cx;
            double srcY = -xt * sinA + yt * cosA + cy;

            int ix = (int)srcX;
            int iy = (int)srcY;

            if (ix < 0 || ix >= width || iy < 0 || iy >= height)
                dest[y * width + x] = 255;     // white background
            else
                dest[y * width + x] = src[iy * width + ix];
        }
    }
}


double find_best_angle(unsigned char* gray, int width, int height)
{
	double best_angle = 0.0;
	double best_score = -1.0;

	unsigned char *temp = malloc(width *height);

	for (double angle = -10.0; angle <= 10.0; angle +=0.5)
	{
		rotate_image(gray,temp, width, height, angle);
		double score = compute_variance_score(temp,width,height);
		
		if (score > best_score)
		{
			best_score = score;
			best_angle = angle;
		}


	}
	free(temp);
	return best_angle;





}





