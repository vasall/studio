#include <asset/parser.h>

#include <devkit/log/log.h>

/*
 * stb_image is compiled here and only here. Files are read by the loader,
 * so its own file functions aren't needed. Images larger than this are
 * rejected, which also keeps width * height far away from overflowing.
 */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_MAX_DIMENSIONS 16384

/* stb_image is third-party code, don't drown our warnings in its own */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <stb_image.h>
#pragma GCC diagnostic pop

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 parse_image(const u8 *data, s32 size, Image *img)
{
        u8 *pixels;
        int w;
        int h;
        int channels;
        s64 i;

        if(!data || size < 1 || !img) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        /* Always ask for 4 channels (RGBA), whatever the file contains */
        pixels = stbi_load_from_memory(data, size, &w, &h, &channels, 4);
        if(!pixels) {
                LOG_S(LOG_ERROR, "Failed to decode image: %s",
                                (char *)stbi_failure_reason());
                return -1;
        }

        if(img_init(img, w, h) < 0) {
                stbi_image_free(pixels);
                return -1;
        }

        /* All images store premultiplied alpha */
        for(i = 0; i < img->pixel_count; i++) {
                IColor col;

                col.r = pixels[i * 4 + 0];
                col.g = pixels[i * 4 + 1];
                col.b = pixels[i * 4 + 2];
                col.a = pixels[i * 4 + 3];
                img->pixels[i] = icolor_premultiply(col);
        }

        stbi_image_free(pixels);
        return 0;
}
