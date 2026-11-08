#ifndef VASALL_UI_SCREEN_H
#define VASALL_UI_SCREEN_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <asset/asset_cache.h>

typedef struct UIScreen {
        AssetCache *assets;

        s32 width;
        s32 height;
} UIScreen;

/*
 *
*/
PUB s32 ui_screen_init(UIScreen *screen, AssetCache *cache, s32 w, s32 h);

/*
 *
*/
PUB void ui_screen_deinit(UIScreen *screen);

/*
 *
*/
PUB void ui_screen_resize(UIScreen *screen, s32 w, s32 h);

#endif /* VASALL_UI_SCREEN_H */