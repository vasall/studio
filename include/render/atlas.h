#ifndef VASALL_STUDIO_ATLAS_H
#define VASALL_STUDIO_ATLAS_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/list.h>
#include <devkit/memory/hashmap.h>

#include <glad/gl.h>


/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * One cached bitmap (glyph or icon) in the atlas.
 */
typedef struct AtlasEntry {
        u16 x;                  /* Position in the atlas, pixels */
        u16 y;
        u16 w;
        u16 h;
        f32 xoff;               /* Offset of the bitmap from the pen/origin */
        f32 yoff;
        f32 advance;            /* Glyphs: pen advance in pixels */
} AtlasEntry;

/*
 * A single-channel texture that glyphs and icons are packed into on first
 * use, row by row ("shelves"). When it is full it is flushed and refilled.
 */
typedef struct Atlas {
        GLuint texture;
        s32 size;               /* Width and height in pixels */

        s32 shelf_x;            /* Next free x in the current shelf */
        s32 shelf_y;            /* Top of the current shelf */
        s32 shelf_h;            /* Height of the current shelf */

        List entries;           /* AtlasEntry */
        HashMap index;          /* key -> entry index + 1 */
        s32 flushed;            /* Set when a flush happened this frame */
} Atlas;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

PUB s32 atlas_init(Atlas *atlas, s32 size);
PUB void atlas_deinit(Atlas *atlas);

/*
 * Find a cached entry, or NULL.
 */
PUB const AtlasEntry *atlas_find(Atlas *atlas, u64 key);

/*
 * Add a bitmap of w x h bytes (one byte per pixel) under key. If it doesn't
 * fit, the atlas is flushed first (all old entries are gone).
 *
 * @returns The entry, or NULL if the bitmap is bigger than the atlas
 */
PUB const AtlasEntry *atlas_add(Atlas *atlas, u64 key, s32 w, s32 h,
                const u8 *pixels, f32 xoff, f32 yoff, f32 advance);

/*
 * Remove all entries.
 */
PUB void atlas_flush(Atlas *atlas);

#endif /* VASALL_STUDIO_ATLAS_H */
