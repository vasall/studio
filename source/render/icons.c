#include <render/icons.h>

#include <devkit/log/log.h>

#include <stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Must match IconId */
LOC const char *s_names[ICON_COUNT] = {"",
                                       "close",
                                       "dot",
                                       "chevron_right",
                                       "chevron_down",
                                       "split_horizontal",
                                       "split_vertical",
                                       "files",
                                       "search",
                                       "gear",
                                       "warning",
                                       "error",
                                       "info",
                                       "terminal",
                                       "more",
                                       "layout_sidebar",
                                       "layout_panel",
                                       "add",
                                       "cube"};

#define ICON_KEY_ID 0xffULL

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC s32 s_load(IconImage *img, const char *path);
LOC void s_resample(const IconImage *src, u8 *dst, s32 size);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 icons_load(Icons *icons, const char *dir)
{
        char path[512];
        s32 missing = 0;
        s32 i;

        memset(icons, 0, sizeof(*icons));

        for(i = 1; i < ICON_COUNT; i++) {
                snprintf(path, sizeof(path), "%s/%s_16.png", dir, s_names[i]);
                if(s_load(&icons->small[i], path) < 0) {
                        LOG_S(LOG_WARN, "icons: failed to load %s", path);
                        missing++;
                }
                snprintf(path, sizeof(path), "%s/%s_32.png", dir, s_names[i]);
                if(s_load(&icons->large[i], path) < 0) {
                        LOG_S(LOG_WARN, "icon: failed to load %s", path);
                        missing++;
                }
        }
        if(missing) {
                LOG_I(LOG_WARN, "icons: %d files missing", missing);
        }

        return 0;
}

PUB void icons_unload(Icons *icons)
{
        s32 i;

        for(i = 0; i < ICON_COUNT; i++) {
                free(icons->small[i].alpha);
                free(icons->large[i].alpha);
        }
        memset(icons, 0, sizeof(*icons));
}

PUB const AtlasEntry *icons_get(Icons *icons, Atlas *atlas, u16 id, s32 px)
{
        const AtlasEntry *e;
        const IconImage *src;
        u64 key;
        u8 *buf;

        if(id == ICON_NONE || id >= ICON_COUNT || px <= 0) {
                return NULL;
        }

        key = (ICON_KEY_ID << 56) | ((u64)px << 32) | id;
        if((e = atlas_find(atlas, key))) return e;

        /* The source at or above the size, so we only scale down */
        src = px <= 16 ? &icons->small[id] : &icons->large[id];
        if(!src->alpha) {
                src = icons->large[id].alpha ? &icons->large[id] :
                                               &icons->small[id];
        }
        if(!src->alpha) return NULL;

        if(!(buf = malloc((size_t)px * (size_t)px))) {
                LOG(LOG_ERROR, "Failed to allocate temp buffer");
                return NULL;
        }

        s_resample(src, buf, px);
        e = atlas_add(atlas, key, px, px, buf, 0.0f, 0.0f, 0.0f);
        free(buf);
        return e;
}

PUB const char *icons_name(u16 id)
{
        return id < ICON_COUNT ? s_names[id] : "";
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC s32 s_load(IconImage *img, const char *path)
{
        s32 w;
        s32 h;
        s32 n;
        s32 i;
        u8 *rgba;

        if(!(rgba = stbi_load(path, &w, &h, &n, 4))) {
                LOG_S(LOG_WARN, "stbi failed to load icon %s", path);
                return -1;
        }

        if(w != h) {
                LOG_S(LOG_WARN, "icons: %s is not square", (char *)path);
                stbi_image_free(rgba);
                return -1;
        }

        if(!(img->alpha = malloc((size_t)w * (size_t)h))) {
                LOG(LOG_ERROR, "Failed to allocate memory for alpha channel");

                stbi_image_free(rgba);
                return -1;
        }

        /* Coverage is the alpha channel; the color comes from the theme */
        for(i = 0; i < w * h; i++) {
                img->alpha[i] = rgba[i * 4 + 3];
        }

        img->size = w;
        stbi_image_free(rgba);
        return 0;
}

/* Area-average resampling to size x size */
LOC void s_resample(const IconImage *src, u8 *dst, s32 size)
{
        f32 ratio = (f32)src->size / (f32)size;
        s32 x;
        s32 y;
        s32 sx;
        s32 sy;

        if(src->size == size) {
                memcpy(dst, src->alpha, (size_t)size * (size_t)size);
                return;
        }

        for(y = 0; y < size; y++) {
                for(x = 0; x < size; x++) {
                        f32 x0 = (f32)x * ratio;
                        f32 y0 = (f32)y * ratio;
                        f32 x1 = x0 + ratio;
                        f32 y1 = y0 + ratio;
                        f32 sum = 0.0f;
                        f32 area = 0.0f;

                        for(sy = (s32)y0;
                            sy < (s32)(y1 + 0.999f) && sy < src->size; sy++) {
                                f32 cy0 = (f32)sy > y0 ? (f32)sy : y0;
                                f32 cy1 =
                                        (f32)(sy + 1) < y1 ? (f32)(sy + 1) : y1;
                                for(sx = (s32)x0;
                                    sx < (s32)(x1 + 0.999f) && sx < src->size;
                                    sx++) {
                                        f32 cx0 = (f32)sx > x0 ? (f32)sx : x0;
                                        f32 cx1 = (f32)(sx + 1) < x1 ?
                                                          (f32)(sx + 1) :
                                                          x1;
                                        f32 a = (cx1 - cx0) * (cy1 - cy0);
                                        if(a <= 0.0f) continue;
                                        sum += a *
                                               (f32)src->alpha[sy * src->size +
                                                               sx];
                                        area += a;
                                }
                        }
                        dst[y * size + x] =
                                (u8)(area > 0.0f ? sum / area + 0.5f : 0.0f);
                }
        }
}
