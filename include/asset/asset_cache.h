#ifndef VASALL_ASSET_CACHE_H
#define VASALL_ASSET_CACHE_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/pool.h>
#include <devkit/memory/text.h>

#include <graphic/image/image.h>
#include <graphic/font/font.h>
#include <graphic/font/font_family.h>

#include <asset/file_handle.h>
#include <asset/loader.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

#define ASSET_PATH_LENGTH       LOADER_PATH_LENGTH      /* Asset path   */
#define ASSET_BASE_DIR_LENGTH   LOADER_BASE_DIR_LENGTH  /* Base dir     */

/*
 * Check if an ImageId, FontId, TextId or FontFamilyId was handed out, i.e.
 * the load didn't fail. Failed loads return index -1.
 */
#define ASSET_ID_VALID(id) POOL_HANDLE_VALID(id)

/*
 * Turn any typed asset ID into a PoolHandle, for the functions that work
 * with all asset types, e.g. asset_version(cache, ASSET_HANDLE(img)).
 */
#define ASSET_HANDLE(id) pool_handle((id).index, (id).generation)

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef enum AssetType {
        ASSET_NONE = 0,
        ASSET_IMAGE,
        ASSET_FONT,
        ASSET_FONT_FAMILY,      /* Made in code, path is its name */
        ASSET_TEXT              /* Shader source, config files */
} AssetType;

/* Separate ID types, so the compiler catches mix-ups */
typedef struct ImageId { s32 index; u32 generation; } ImageId;
typedef struct FontId { s32 index; u32 generation; } FontId;
typedef struct TextId { s32 index; u32 generation; } TextId;
typedef struct FontFamilyId { s32 index; u32 generation; } FontFamilyId;

typedef struct Asset {
        AssetType type;
        char path[ASSET_PATH_LENGTH];   /* Relative to the base dir */

        FileHandle file;        /* To notice when the file changes */
        u32 version;            /* 1 when loaded, +1 on every reload */

        union {
                Image image;
                Font font;
                FontFamily family;
                Text text;
        } data;
} Asset;

/*
 * Owns loaded files and shares them. Every asset stays loaded until the cache
 * is freed with asset_deinit. Loading goes through two steps:
 *
 *   loader  reads the raw bytes of the file      (asset/loader.h)
 *   parser  turns them into an Image, Font, Text (asset/parser.h)
 *
 * The assets live in a pool that never moves them, so a pointer to an
 * asset's data stays valid until asset_deinit, also across reloads.
 *
 * Use one cache per owner and lifetime, e.g. one for studio's own assets and
 * one for the open project.
 */
typedef struct AssetCache {
        Loader loader;
        Pool assets;            /* Pool of Asset */
} AssetCache;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Initialize an empty cache. Asset paths are relative to base_dir.
 *
 * @param cache Pointer to the cache
 * @param base_dir The folder the assets are loaded from
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 asset_init(AssetCache *cache, const char *base_dir);

/*
 * Free all assets and the cache. All asset IDs and pointers of this cache
 * become invalid.
 */
PUB void asset_deinit(AssetCache *cache);

/*
 * Load an asset, or get the ID of the already loaded one. The path is
 * relative to the base dir, uses '/' and must not start with '/' or contain
 * "..", so assets always stay inside the base dir.
 *
 * @param cache Pointer to the cache
 * @param path The path of the file, e.g. "icons/save.png"
 *
 * @returns The ID of the asset, with index -1 if the file could not be
 *          loaded (check with ASSET_ID_VALID)
 */
PUB ImageId asset_load_image(AssetCache *cache, const char *path);
PUB FontId asset_load_font(AssetCache *cache, const char *path);
PUB TextId asset_load_text(AssetCache *cache, const char *path);

/*
 * Get the data of an asset. Returns NULL if the ID is invalid.
 *
 * The pointer stays valid until asset_deinit, also when other assets are
 * loaded and when the asset is reloaded, so it can be kept.
 */
PUB Image *asset_image(AssetCache *cache, ImageId id);
PUB Font *asset_font(AssetCache *cache, FontId id);
PUB FontFamily *asset_font_family(AssetCache *cache, FontFamilyId id);
PUB Text *asset_text(AssetCache *cache, TextId id);

/*
 * Create an empty font family, or get the existing one with this name. Add
 * fonts to it with asset_family_add.
 *
 * @param cache Pointer to the cache
 * @param name The name of the family, e.g. "roboto"
 *
 * @returns The ID of the family, with index -1 if an error occurred
 */
PUB FontFamilyId asset_create_font_family(AssetCache *cache, const char *name);

/*
 * Load a font and use it for one style of a family. A style that is already
 * set is replaced. The font is a normal font asset, so loading the same file
 * for another family shares it. If the font can't be loaded, the style stays
 * as it was.
 *
 * @param cache Pointer to the cache
 * @param family The ID of the family
 * @param style The style the font is for
 * @param path The path of the font file, like for asset_load_font
 *
 * @returns 0 on success, -1 if an error occurred
 */
PUB s32 asset_family_add(AssetCache *cache, FontFamilyId family,
                         FontStyle style, const char *path);

/*
 * Get the number of loaded assets.
 */
PUB s32 asset_count(AssetCache *cache);

/*
 * Load every asset again whose file changed on disk. If the new file can't
 * be read or parsed (e.g. an editor is still writing it), the old data stays.
 * It is tried again once the file changes again, or on the next call if the
 * file couldn't be read at all.
 *
 * The data is replaced in place, so pointers to assets stay valid. Pointers
 * into the old data don't, e.g. GlyphCache pointers of a reloaded font or
 * the pixels of a reloaded image. Things made from an asset, like a texture
 * from an image, have to be made again; compare asset_version to notice.
 *
 * @returns The number of reloaded assets
 */
PUB s32 asset_reload_changed(AssetCache *cache);

/*
 * Get how often an asset was loaded: 1 after the first load, +1 for every
 * reload. Returns 0 if the handle is invalid. Use ASSET_HANDLE for typed IDs.
 */
PUB u32 asset_version(AssetCache *cache, PoolHandle handle);

#endif /* VASALL_ASSET_CACHE_H */
