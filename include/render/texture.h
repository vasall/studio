#ifndef VASALL_TEXTURE_H
#define VASALL_TEXTURE_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <shape/rectangle.h>
#include <graphic/general/color.h>
#include <graphic/image/image.h>

#include <glad/gl.h>

/*
 * An RGBA texture on the GPU. Pixel row 0 is the top row of the image, like
 * in Image, so shaders have to flip v when they want the image upright.
 */
typedef struct Texture {
        GLuint id;
        s32 width;
        s32 height;
} Texture;

/*
 * Create a texture. Needs a current GL context.
 *
 * @param tex Pointer to the texture
 * @param w The width in pixels
 * @param h The height in pixels
 * @param px w * h pixels, row by row from the top, or NULL to leave the
 *           texture uninitialized
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 texture_init(Texture *tex, s32 w, s32 h, IColor *px);

/*
 * Delete the texture.
 */
PUB void texture_deinit(Texture *tex);

/*
 * Copy the pixels inside rect from the image to the same position in the
 * texture. The rect is clipped to both the image and the texture, so only
 * the overlapping part is copied.
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 texture_update(Texture *tex, Image *img, Rectangle rect);

#endif /* VASALL_TEXTURE_H */
