#include <asset/asset_cache.h>

#include <devkit/log/log.h>

#include "loader.h"

#include <stdio.h>
#include <string.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * Check that the path is relative, not empty, short enough and doesn't leave
 * the base dir with "..".
 *
 * @returns 0 if the path can be used, -1 if not
 */
LOC s32 a_check_path(const char *path);

/*
 * Build the full path: base_dir + '/' + path.
 *
 * @returns 0 on success, -1 if the result doesn't fit into buf
 */
LOC s32 a_full_path(AssetCache *cache, const char *path, char *buf, s32 size);

/*
 * Get the asset with the given ID if it has the expected type.
 *
 * @returns The asset, or NULL if the ID is invalid or the type differs
 */
LOC Asset *a_get(AssetCache *cache, AssetId id, AssetType type);

/*
 * Find the asset or load it with the loader for its type.
 *
 * @returns The ID of the asset, or -1 if it could not be loaded
 */
LOC AssetId a_load(AssetCache *cache, AssetType type, const char *path);

/* Free the data of one asset */
LOC void a_free(Asset *asset);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 asset_init(AssetCache *cache, const char *base_dir)
{
        size_t len;

        if(!cache || !base_dir) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if((len = strlen(base_dir)) >= sizeof(cache->base_dir)) {
                LOG(LOG_ERROR, "Asset base dir is too long");
                return -1;
        }

        if(list_init(&cache->assets, sizeof(Asset), 32) < 0) {
                LOG(LOG_ERROR, "Failed to initialize asset list");
                return -1;
        }

        memcpy(cache->base_dir, base_dir, len + 1);

        /* Without a trailing '/', so joining always adds exactly one */
        while(len > 1 && cache->base_dir[len - 1] == '/') {
                cache->base_dir[--len] = '\0';
        }

        return 0;
}

PUB void asset_deinit(AssetCache *cache)
{
        s32 i;

        if(!cache) {
                return;
        }

        for(i = 0; i < list_count(&cache->assets); i++) {
                a_free(list_get(&cache->assets, i));
        }

        list_deinit(&cache->assets);
}

PUB AssetId asset_load_image(AssetCache *cache, const char *path)
{
        return a_load(cache, ASSET_IMAGE, path);
}

PUB AssetId asset_load_font(AssetCache *cache, const char *path)
{
        return a_load(cache, ASSET_FONT, path);
}

PUB AssetId asset_load_text(AssetCache *cache, const char *path)
{
        return a_load(cache, ASSET_TEXT, path);
}

PUB AssetId asset_find(AssetCache *cache, AssetType type, const char *path)
{
        s32 i;

        if(!cache || !path) {
                return -1;
        }

        /* Linear search; it only runs when loading, not every frame */
        for(i = 0; i < list_count(&cache->assets); i++) {
                Asset *a = list_get(&cache->assets, i);

                if(a->type == type && strcmp(a->path, path) == 0) {
                        return i;
                }
        }

        return -1;
}

PUB Image *asset_image(AssetCache *cache, AssetId id)
{
        Asset *a = a_get(cache, id, ASSET_IMAGE);
        return a ? &a->data.image : NULL;
}

PUB Font *asset_font(AssetCache *cache, AssetId id)
{
        Asset *a = a_get(cache, id, ASSET_FONT);
        return a ? &a->data.font : NULL;
}

PUB Text *asset_text(AssetCache *cache, AssetId id)
{
        Asset *a = a_get(cache, id, ASSET_TEXT);
        return a ? &a->data.text : NULL;
}

PUB s32 asset_count(AssetCache *cache)
{
        return cache ? list_count(&cache->assets) : 0;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC s32 a_check_path(const char *path)
{
        const char *seg = path;
        size_t      len = strlen(path);

        if(len == 0 || len >= ASSET_PATH_LENGTH) {
                return -1;
        }

        if(path[0] == '/') {
                return -1;
        }

        /* Check every segment between the '/' for ".." */
        while(*seg) {
                const char *end = strchr(seg, '/');
                size_t      seg_len = end ? (size_t)(end - seg) : strlen(seg);

                if(seg_len == 2 && seg[0] == '.' && seg[1] == '.') {
                        return -1;
                }

                if(!end) {
                        break;
                }
                seg = end + 1;
        }

        return 0;
}

LOC s32 a_full_path(AssetCache *cache, const char *path, char *buf, s32 size)
{
        s32 n = snprintf(buf, size, "%s/%s", cache->base_dir, path);

        return (n < 0 || n >= size) ? -1 : 0;
}

LOC Asset *a_get(AssetCache *cache, AssetId id, AssetType type)
{
        Asset *a;

        if(!cache || id < 0 || id >= list_count(&cache->assets)) {
                LOG_I(LOG_ERROR, "Invalid asset ID %d", id);
                return NULL;
        }

        a = list_get(&cache->assets, id);
        if(a->type != type) {
                LOG_I(LOG_ERROR, "Asset %d has a different type", id);
                return NULL;
        }

        return a;
}

LOC AssetId a_load(AssetCache *cache, AssetType type, const char *path)
{
        char    full[ASSET_BASE_DIR_LENGTH + ASSET_PATH_LENGTH + 1];
        Asset   asset;
        AssetId id;
        s32     ret = -1;

        if(!cache || !path) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        /* Already loaded: share it */
        if((id = asset_find(cache, type, path)) >= 0) {
                return id;
        }

        if(a_check_path(path) < 0) {
                LOG_S(LOG_ERROR, "Invalid asset path: %s", (char *)path);
                return -1;
        }

        if(a_full_path(cache, path, full, sizeof(full)) < 0) {
                LOG_S(LOG_ERROR, "Asset path is too long: %s", (char *)path);
                return -1;
        }

        memset(&asset, 0, sizeof(asset));
        asset.type = type;
        strcpy(asset.path, path); /* Length checked in a_check_path */

        switch(type) {
        case ASSET_IMAGE: ret = load_image(full, &asset.data.image); break;
        case ASSET_FONT: ret = load_font(full, &asset.data.font); break;
        case ASSET_TEXT: ret = load_text(full, &asset.data.text); break;
        default: break;
        }

        if(ret < 0) {
                return -1;
        }

        /* The list stores a copy; the loaded data now belongs to the cache */
        id = list_count(&cache->assets);
        if(list_push(&cache->assets, 1, &asset) < 0) {
                LOG(LOG_ERROR, "Failed to add asset to cache");
                a_free(&asset);
                return -1;
        }

        return id;
}

LOC void a_free(Asset *asset)
{
        switch(asset->type) {
        case ASSET_IMAGE: img_deinit(&asset->data.image); break;
        case ASSET_FONT: font_deinit(&asset->data.font); break;
        case ASSET_TEXT: text_deinit(&asset->data.text); break;
        default: break;
        }

        asset->type = ASSET_NONE;
}
