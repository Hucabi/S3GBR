#include "image_loader.h"
#include "image_utils.h"
#include "stb_image.h"
#include "stb_image_write.h"
Image LoadImage(const char* path) {
    Image img;
    img.data = stbi_load(path, &img.width, &img.height, &img.channels, 0);

    if (img.data == NULL) {
        fprintf(stderr, "Error: Could not load image '%s'\n", path);
        img.width = img.height = img.channels = 0;
    } else {
        printf("Image loaded successfully!\n");
        printf("Dimensions: %d x %d\n", img.width, img.height);
        printf("Channels: %d\n", img.channels);
    }

    return img;
}

void FreeImage(Image img) {
    stbi_image_free(img.data);
}
