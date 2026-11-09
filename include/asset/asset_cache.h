#ifndef VASALL_ASSET_CACHE_H
#define VASALL_ASSET_CACHE_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/list.h>
#include <devkit/memory/text.h>

#include <graphic/image/image.h>
#include <graphic/font/font.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

#define ASSET_PATH_LENGTH       128     /* Max. length of an asset path   */
#define ASSET_BASE_DIR_LENGTH   256     /* Max. length of the base dir    */

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef enum AssetType {
        ASSET_NONE = 0,
        ASSET_IMAGE,
        ASSET_FONT,
        ASSET_TEXT              /* Shader source, config files */
} AssetType;

typedef struct Asset {
        AssetType type;
        char path[ASSET_PATH_LENGTH];   /* Relative to the base dir */

        union {
                Image image;
                Font font;
                Text text;
        } data;
} Asset;

/*
 * Loads files once and shares them. Every asset stays loaded until the cache
 * is freed with asset_deinit.
 *
 * Use one cache per owner and lifetime, e.g. one for studio's own assets and
 * one for the open project.
 */
typedef struct AssetCache {
        char base_dir[ASSET_BASE_DIR_LENGTH];
        List assets;            /* List of Asset */
} AssetCache;

/*
 * Identifies an asset within one cache. The same number means a different
 * asset in another cache. -1 means "no asset".
 */
typedef s32 AssetId;

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
 * Free all assets and the cache. All asset IDs of this cache become invalid.
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
 * @returns The ID of the asset, or -1 if the file could not be loaded
 */
PUB AssetId asset_load_image(AssetCache *cache, const char *path);
PUB AssetId asset_load_font(AssetCache *cache, const char *path);
PUB AssetId asset_load_text(AssetCache *cache, const char *path);

/*
 * Find an already loaded asset without loading it.
 *
 * @returns The ID of the asset, or -1 if it is not loaded
 */
PUB AssetId asset_find(AssetCache *cache, AssetType type, const char *path);

/*
 * Get the data of an asset. Returns NULL if the ID is invalid or the asset
 * has a different type.
 *
 * The pointer is only valid until the next asset is loaded into this cache,
 * because the cache may have to move its memory. Keep the AssetId and call
 * these again when the data is needed.
 */
PUB Image *asset_image(AssetCache *cache, AssetId id);
PUB Font *asset_font(AssetCache *cache, AssetId id);
PUB Text *asset_text(AssetCache *cache, AssetId id);

/*
 * Get the number of loaded assets. Valid IDs are 0 .. count - 1.
 */
PUB s32 asset_count(AssetCache *cache);

#endif /* VASALL_ASSET_CACHE_H */
