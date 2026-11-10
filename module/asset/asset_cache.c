#include <asset/asset_cache.h>

#include <asset/parser.h>

#include <devkit/log/log.h>

#include <stdlib.h>
#include <string.h>

/* Assets allocated at once in the pool */
#define ASSET_CHUNK_ITEMS       32

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * Get the asset of a handle if it has the expected type.
 *
 * @returns The asset, or NULL if the handle is invalid or the type differs
 */
LOC Asset *a_get(AssetCache *cache, PoolHandle h, AssetType type);

/*
 * Find an already loaded asset by type and path (or family name).
 *
 * @returns The handle, with index -1 if it isn't loaded
 */
LOC PoolHandle a_find(AssetCache *cache, AssetType type, const char *path);

/*
 * Find the asset or load it through the loader and the parser.
 *
 * @returns The handle, with index -1 if it could not be loaded
 */
LOC PoolHandle a_load(AssetCache *cache, AssetType type, const char *path);

/*
 * Read the asset's file and parse it into data, by the asset's type. The
 * asset itself isn't changed, so a failed reload keeps the old data.
 *
 * @returns 0 on success, -1 if an error occurred
 */
LOC s32 a_read(AssetCache *cache, Asset *asset, void *data);

/* Free the data of an asset, by its type */
LOC void a_free_data(AssetType type, void *data);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 asset_init(AssetCache *cache, const char *base_dir)
{
        if(!cache || !base_dir) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        if(loader_init(&cache->loader, base_dir) < 0) {
                return -1;
        }

        if(pool_init(&cache->assets, sizeof(Asset), ASSET_CHUNK_ITEMS) < 0) {
                LOG(LOG_ERROR, "Failed to initialize asset pool");
                return -1;
        }

        return 0;
}

PUB void asset_deinit(AssetCache *cache)
{
        s32 i;

        if(!cache) {
                return;
        }

        for(i = 0; i < pool_slots(&cache->assets); i++) {
                Asset *a = pool_at(&cache->assets, i, NULL);

                if(a) {
                        a_free_data(a->type, &a->data);
                }
        }

        pool_deinit(&cache->assets);
}

PUB ImageId asset_load_image(AssetCache *cache, const char *path)
{
        PoolHandle h = a_load(cache, ASSET_IMAGE, path);
        ImageId id;

        id.index = h.index;
        id.generation = h.generation;
        return id;
}

PUB FontId asset_load_font(AssetCache *cache, const char *path)
{
        PoolHandle h = a_load(cache, ASSET_FONT, path);
        FontId id;

        id.index = h.index;
        id.generation = h.generation;
        return id;
}

PUB TextId asset_load_text(AssetCache *cache, const char *path)
{
        PoolHandle h = a_load(cache, ASSET_TEXT, path);
        TextId id;

        id.index = h.index;
        id.generation = h.generation;
        return id;
}

PUB Image *asset_image(AssetCache *cache, ImageId id)
{
        Asset *a = a_get(cache, ASSET_HANDLE(id), ASSET_IMAGE);
        return a ? &a->data.image : NULL;
}

PUB Font *asset_font(AssetCache *cache, FontId id)
{
        Asset *a = a_get(cache, ASSET_HANDLE(id), ASSET_FONT);
        return a ? &a->data.font : NULL;
}

PUB FontFamily *asset_font_family(AssetCache *cache, FontFamilyId id)
{
        Asset *a = a_get(cache, ASSET_HANDLE(id), ASSET_FONT_FAMILY);
        return a ? &a->data.family : NULL;
}

PUB Text *asset_text(AssetCache *cache, TextId id)
{
        Asset *a = a_get(cache, ASSET_HANDLE(id), ASSET_TEXT);
        return a ? &a->data.text : NULL;
}

PUB FontFamilyId asset_create_font_family(AssetCache *cache, const char *name)
{
        FontFamilyId id = { POOL_HANDLE_NONE };
        PoolHandle h;
        Asset *a;

        if(!cache || !name || name[0] == '\0') {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return id;
        }

        h = a_find(cache, ASSET_FONT_FAMILY, name);
        if(!POOL_HANDLE_VALID(h)) {
                if(strlen(name) >= ASSET_PATH_LENGTH) {
                        LOG_S(LOG_ERROR, "Font family name is too long: %s",
                              (char *)name);
                        return id;
                }

                /* Zeroed: file.known is 0, so reloading skips the family */
                if(!(a = pool_add(&cache->assets, NULL, &h))) {
                        LOG(LOG_ERROR, "Failed to add font family");
                        return id;
                }

                a->type = ASSET_FONT_FAMILY;
                a->version = 1;
                strcpy(a->path, name);
                font_family_init(&a->data.family);
        }

        id.index = h.index;
        id.generation = h.generation;
        return id;
}

PUB s32 asset_family_add(AssetCache *cache, FontFamilyId family,
                         FontStyle style, const char *path)
{
        FontFamily *fam;
        FontId font;

        if(!cache || !path || style < 0 || style >= FONT_STYLE_COUNT) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return -1;
        }

        /* Check the family first, so a wrong ID doesn't load a font */
        if(!(fam = asset_font_family(cache, family))) {
                return -1;
        }

        font = asset_load_font(cache, path);
        if(!ASSET_ID_VALID(font)) {
                return -1;
        }

        /* Both pointers stay valid: the pool never moves assets */
        return font_family_set(fam, style, asset_font(cache, font));
}

PUB s32 asset_count(AssetCache *cache)
{
        return cache ? pool_count(&cache->assets) : 0;
}

PUB s32 asset_reload_changed(AssetCache *cache)
{
        s32 reloaded = 0;
        s32 i;

        if(!cache) {
                return 0;
        }

        for(i = 0; i < pool_slots(&cache->assets); i++) {
                Asset *a = pool_at(&cache->assets, i, NULL);
                Asset fresh;

                /* -1 (e.g. deleted while saving) is checked again next time */
                if(!a || file_changed(&a->file) != 1) {
                        continue;
                }

                /* Parse into a copy, so a failure leaves the old data */
                fresh = *a;
                if(a_read(cache, &fresh, &fresh.data) < 0) {
                        LOG_S(LOG_WARN, "Keeping old version of %s", a->path);

                        /* Don't retry until the file changes again */
                        a->file = fresh.file;
                        continue;
                }

                /* Replace the data in place, so pointers to it stay valid */
                a_free_data(a->type, &a->data);
                a->data = fresh.data;
                a->file = fresh.file;
                a->version++;
                reloaded++;

                LOG_S(LOG_INFO, "Reloaded %s", a->path);
        }

        return reloaded;
}

PUB u32 asset_version(AssetCache *cache, PoolHandle handle)
{
        Asset *a;

        if(!cache || !(a = pool_get(&cache->assets, handle))) {
                return 0;
        }

        return a->version;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC Asset *a_get(AssetCache *cache, PoolHandle h, AssetType type)
{
        Asset *a;

        if(!cache || !(a = pool_get(&cache->assets, h))) {
                LOG_I(LOG_ERROR, "Invalid asset ID %d", h.index);
                return NULL;
        }

        if(a->type != type) {
                LOG_I(LOG_ERROR, "Asset %d has a different type", h.index);
                return NULL;
        }

        return a;
}

LOC PoolHandle a_find(AssetCache *cache, AssetType type, const char *path)
{
        PoolHandle h;
        s32 i;

        /* Linear search; it only runs when loading, not every frame */
        for(i = 0; i < pool_slots(&cache->assets); i++) {
                Asset *a = pool_at(&cache->assets, i, &h);

                if(a && a->type == type && strcmp(a->path, path) == 0) {
                        return h;
                }
        }

        return pool_handle(-1, 0);
}

LOC PoolHandle a_load(AssetCache *cache, AssetType type, const char *path)
{
        PoolHandle none = pool_handle(-1, 0);
        PoolHandle h;
        Asset asset;

        if(!cache || !path) {
                LOG(LOG_ERROR, "Invalid parameter(s)");
                return none;
        }

        /* Already loaded: share it */
        h = a_find(cache, type, path);
        if(POOL_HANDLE_VALID(h)) {
                return h;
        }

        memset(&asset, 0, sizeof(asset));

        /* Checks the path, so it fits into asset.path afterwards */
        if(loader_open(&cache->loader, path, &asset.file) < 0) {
                return none;
        }

        asset.type = type;
        asset.version = 1;
        strcpy(asset.path, path);

        if(a_read(cache, &asset, &asset.data) < 0) {
                return none;
        }

        /* The pool stores a copy; the loaded data now belongs to the cache */
        if(!pool_add(&cache->assets, &asset, &h)) {
                LOG(LOG_ERROR, "Failed to add asset to cache");
                a_free_data(type, &asset.data);
                return none;
        }

        return h;
}

LOC s32 a_read(AssetCache *cache, Asset *asset, void *data)
{
        u8 *file;
        s32 size;
        s32 ret = -1;

        if(loader_read(&cache->loader, &asset->file, &file, &size) < 0) {
                return -1;
        }

        switch(asset->type) {
        case ASSET_IMAGE:
                ret = parse_image(file, size, data);
                break;

        case ASSET_FONT:
                /* The font keeps the file data on success */
                if((ret = parse_font(file, size, data)) == 0) {
                        file = NULL;
                }
                break;

        case ASSET_TEXT:
                ret = parse_text(file, size, data);
                break;

        default:
                break;
        }

        if(ret < 0) {
                LOG_S(LOG_ERROR, "Failed to parse %s", asset->path);
        }

        free(file);
        return ret;
}

LOC void a_free_data(AssetType type, void *data)
{
        switch(type) {
        case ASSET_IMAGE: img_deinit(data); break;
        case ASSET_FONT: font_deinit(data); break;
        case ASSET_TEXT: text_deinit(data); break;

        /* A family doesn't own its fonts, they are assets themselves */
        default: break;
        }
}
