#include <render/atlas.h>

#include <devkit/log/log.h>

#include <stdlib.h>
#include <string.h>

#define PADDING 1

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 atlas_init(Atlas *atlas, s32 size)
{
        u8 *zero;

        memset(atlas, 0, sizeof(*atlas));
        atlas->size = size;

        if(list_init(&atlas->entries, sizeof(AtlasEntry), 256) < 0) return -1;
        if(hashmap_init(&atlas->index, 256) < 0) {
                list_deinit(&atlas->entries);
                return -1;
        }

        zero = calloc((size_t)size * (size_t)size, 1);
        if(!zero) {
                atlas_deinit(atlas);
                return -1;
        }

        glGenTextures(1, &atlas->texture);
        glBindTexture(GL_TEXTURE_2D, atlas->texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, size, size, 0, GL_RED,
                        GL_UNSIGNED_BYTE, zero);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        free(zero);

        atlas_flush(atlas);
        return 0;
}

PUB void atlas_deinit(Atlas *atlas)
{
        if(atlas->texture) glDeleteTextures(1, &atlas->texture);
        atlas->texture = 0;
        list_deinit(&atlas->entries);
        hashmap_deinit(&atlas->index);
}

PUB const AtlasEntry *atlas_find(Atlas *atlas, u64 key)
{
        u64 idx;

        if(!hashmap_get(&atlas->index, key, &idx)) return NULL;
        return list_get(&atlas->entries, (s32)idx - 1);
}

PUB const AtlasEntry *atlas_add(Atlas *atlas, u64 key, s32 w, s32 h,
                const u8 *pixels, f32 xoff, f32 yoff, f32 advance)
{
        AtlasEntry e;
        s32 pw = w + PADDING;
        s32 ph = h + PADDING;

        if(pw > atlas->size || ph > atlas->size) return NULL;

        /* Next shelf if the row is full; flush if the atlas is full */
        if(atlas->shelf_x + pw > atlas->size) {
                atlas->shelf_y += atlas->shelf_h;
                atlas->shelf_x = PADDING;
                atlas->shelf_h = 0;
        }
        if(atlas->shelf_y + ph > atlas->size) {
                LOG(LOG_INFO, "glyph atlas full, flushing");
                atlas_flush(atlas);
                atlas->flushed = 1;
        }

        e.x = (u16)atlas->shelf_x;
        e.y = (u16)atlas->shelf_y;
        e.w = (u16)w;
        e.h = (u16)h;
        e.xoff = xoff;
        e.yoff = yoff;
        e.advance = advance;

        atlas->shelf_x += pw;
        if(ph > atlas->shelf_h) atlas->shelf_h = ph;

        if(w > 0 && h > 0 && pixels) {
                glBindTexture(GL_TEXTURE_2D, atlas->texture);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexSubImage2D(GL_TEXTURE_2D, 0, e.x, e.y, w, h, GL_RED,
                                GL_UNSIGNED_BYTE, pixels);
        }

        if(list_push(&atlas->entries, 1, &e) < 0) return NULL;
        hashmap_set(&atlas->index, key, (u64)list_count(&atlas->entries));
        return list_get(&atlas->entries, list_count(&atlas->entries) - 1);
}

PUB void atlas_flush(Atlas *atlas)
{
        list_clear(&atlas->entries);
        hashmap_clear(&atlas->index);
        atlas->shelf_x = PADDING;
        atlas->shelf_y = PADDING;
        atlas->shelf_h = 0;
}
