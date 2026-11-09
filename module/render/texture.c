#include <render/texture.h>

#include <devkit/log/log.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 texture_init(Texture *tex, s32 w, s32 h, IColor *px)
{
        if(!tex || w < 1 || h < 1) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        glGenTextures(1, &tex->id);
        if(!tex->id) {
                LOG(LOG_ERROR, "Failed to create texture");
                return -1;
        }

        glBindTexture(GL_TEXTURE_2D, tex->id);

        /* IColor is 4 bytes, so rows are always 4-byte aligned */
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, px);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);

        tex->width = w;
        tex->height = h;

        return 0;
}

PUB void texture_deinit(Texture *tex)
{
        if(!tex) {
                return;
        }

        glDeleteTextures(1, &tex->id);
        tex->id = 0;
        tex->width = 0;
        tex->height = 0;
}

PUB s32 texture_update(Texture *tex, Image *img, Rectangle rect)
{
        Rectangle area;

        if(!tex || !img || !img->pixels) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        /* Only copy what lies inside both the image and the texture */
        if(rect_intersect(rect, mkrect(0, 0, img->width, img->height),
                          &area) < 0 ||
           rect_intersect(area, mkrect(0, 0, tex->width, tex->height),
                          &area) < 0) {
                return 0;
        }

        glBindTexture(GL_TEXTURE_2D, tex->id);

        /* Read the rect straight out of the full image, without a copy */
        glPixelStorei(GL_UNPACK_ROW_LENGTH, img->width);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, area.x);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, area.y);

        glTexSubImage2D(GL_TEXTURE_2D, 0, area.x, area.y, area.w, area.h,
                        GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);

        /* Reset, so other uploads aren't affected */
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

        glBindTexture(GL_TEXTURE_2D, 0);

        return 0;
}
