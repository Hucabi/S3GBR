#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "image_loader.h"
#include "image_utils.h"
#include "grid_detection.h"
#include "word_list_detection.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double row_variance(unsigned char* row, int width)
{
    double sum = 0.0;
    double sum_sq = 0.0;

    for (int i = 0; i < width; i++)
    {
        double v = row[i];
        sum += v;
        sum_sq += v * v;
    }
    double mean = sum / width;
    double mean_sq = sum_sq / width;

    return mean_sq - mean * mean;
}

double compute_variance_score(unsigned char* gray, int width, int height)
{
    double score = 0.0;
    for (int y = 0; y < height; y++)
    {
        unsigned char *row = &gray[y * width];
        score += row_variance(row, width);
    }
    return score;
}

// Improved rotate_image function
void rotate_image(unsigned char* src,
                  unsigned char* dest,
                  int width,
                  int height,
                  double angle_deg)
{
    // Convert to radians (negative for correct direction)
    double angle = -angle_deg * M_PI / 180.0;
    double cosA = cos(angle);
    double sinA = sin(angle);
    
    // Center of rotation
    double cx = width / 2.0;
    double cy = height / 2.0;
    
    // Initialize with white background
    memset(dest, 255, width * height);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Translate to origin
            double xt = x - cx;
            double yt = y - cy;
            
            // Apply rotation
            double srcX = xt * cosA - yt * sinA + cx;
            double srcY = xt * sinA + yt * cosA + cy;
            
            int ix = (int)srcX;
            int iy = (int)srcY;
            
            // Check bounds
            if (ix >= 0 && ix < width && iy >= 0 && iy < height) {
                dest[y * width + x] = src[iy * width + ix];
            }
            // Outside bounds remain white (255)
        }
    }
}

// Save test images for debugging
void save_rotation_test_images(unsigned char* image, int width, int height) {
    printf("\n=== SAVING ROTATION TEST IMAGES ===\n");
    
    unsigned char* buffer = malloc(width * height);
    if (!buffer) {
        printf("Error: Could not allocate buffer for test images\n");
        return;
    }
    
    // Create debug directory
    mkdir("data/debug", 0755);
    
    // Test angles
    double test_angles[] = {-45.0, -30.0, -15.0, -5.0, 0.0, 5.0, 15.0, 30.0, 45.0, 90.0, 180.0, 270.0};
    int num_angles = sizeof(test_angles) / sizeof(test_angles[0]);
    
    for (int i = 0; i < num_angles; i++) {
        double angle = test_angles[i];
        rotate_image(image, buffer, width, height, angle);
        
        // Save the image
        char filename[256];
        snprintf(filename, sizeof(filename), 
                 "data/debug/test_rotated_%+06.1fdeg.png", angle);
        
        // Note: stbi_write_png is called from main.c, we just prepare the data
        // In practice, we'd need to call it here, but for simplicity we'll
        // just compute and display the scores
        
        double score = compute_variance_score(buffer, width, height);
        printf("Angle %+6.1f°: variance score = %10.2f\n", angle, score);
    }
    
    free(buffer);
    printf("Test angles evaluated. Scores shown above.\n");
}

// New improved find_best_angle with full angle detection
double find_best_angle(unsigned char* gray, int width, int height)
{
    printf("\n=== COMPLETE ROTATION ANGLE DETECTION ===\n");
    
    double best_angle = 0.0;
    double best_score = -1.0;
    
    unsigned char *temp = malloc(width * height);
    if (!temp) {
        printf("Error: Memory allocation failed for rotation buffer\n");
        return 0.0;
    }
    
    // PHASE 1: Check small tilt angles (-45° to +45°)
    printf("\nPHASE 1: Checking small tilt angles (-45° to +45°):\n");
    printf("---------------------------------------------------\n");
    
    int checks_done = 0;
    for (double angle = -45.0; angle <= 45.0; angle += 1.0) {
        rotate_image(gray, temp, width, height, angle);
        double score = compute_variance_score(temp, width, height);
        
        if (score > best_score) {
            best_score = score;
            best_angle = angle;
        }
        
        checks_done++;
        if (checks_done % 10 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    printf("\nBest small tilt: %.2f° (score: %.2f)\n", best_angle, best_score);
    
    // PHASE 2: Check major orientations
    printf("\nPHASE 2: Checking major orientations:\n");
    printf("-------------------------------------\n");
    
    double major_angles[] = {0.0, 90.0, 180.0, 270.0};
    const char* major_names[] = {"0° (normal)", "90° (left)", "180° (upside down)", "270° (right)"};
    
    for (int i = 0; i < 4; i++) {
        rotate_image(gray, temp, width, height, major_angles[i]);
        double score = compute_variance_score(temp, width, height);
        
        printf("%s: score = %.2f", major_names[i], score);
        
        // Check if this major orientation is significantly better
        if (score > best_score * 1.15) { // 15% better threshold
            best_score = score;
            best_angle = major_angles[i];
            printf(" <- NEW BEST (%.1f%% better)", 
                   ((score / best_score) - 1.0) * 100.0);
        }
        printf("\n");
    }
    
    // PHASE 3: Fine-tune around the best angle
    printf("\nPHASE 3: Fine-tuning around %.2f°:\n", best_angle);
    printf("---------------------------------\n");
    
    double fine_tune_start = best_angle - 5.0;
    double fine_tune_end = best_angle + 5.0;
    double original_best_angle = best_angle;
    
    for (double angle = fine_tune_start; angle <= fine_tune_end; angle += 0.25) {
        rotate_image(gray, temp, width, height, angle);
        double score = compute_variance_score(temp, width, height);
        
        if (score > best_score) {
            best_score = score;
            best_angle = angle;
        }
    }
    
    free(temp);
    
    printf("\n=== DETECTION COMPLETE ===\n");
    printf("Initial best angle: %.2f°\n", original_best_angle);
    printf("Final best angle:   %.2f°\n", best_angle);
    printf("Best variance score: %.2f\n", best_score);
    
    // Interpretation
    printf("\n=== INTERPRETATION ===\n");
    if (fabs(best_angle) < 2.0) {
        printf("Text is essentially horizontal (no significant rotation)\n");
    } else if (fabs(best_angle - 180.0) < 2.0 || fabs(best_angle + 180.0) < 2.0) {
        printf("Text is UPSIDE DOWN\n");
    } else if (fabs(best_angle - 90.0) < 2.0) {
        printf("Text is rotated 90° COUNTER-CLOCKWISE (left)\n");
    } else if (fabs(best_angle + 90.0) < 2.0 || fabs(best_angle - 270.0) < 2.0) {
        printf("Text is rotated 90° CLOCKWISE (right)\n");
    } else if (best_angle > 0) {
        printf("Text is tilted COUNTER-CLOCKWISE by %.1f°\n", best_angle);
    } else {
        printf("Text is tilted CLOCKWISE by %.1f°\n", -best_angle);
    }
    
    return best_angle;
}
