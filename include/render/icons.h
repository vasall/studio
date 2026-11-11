#ifndef VASALL_STUDIO_ICONS_H
#define VASALL_STUDIO_ICONS_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <render/atlas.h>
#include <render/icon_ids.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct IconImage {
        s32 size;               /* Width = height in pixels, 0 if missing */
        u8 *alpha;              /* size * size coverage values */
} IconImage;

/*
 * The icon masks at both source sizes, kept on the CPU. The atlas gets a
 * copy resampled to the exact pixel size that is drawn.
 */
typedef struct Icons {
        IconImage small[ICON_COUNT];    /* 16 px */
        IconImage large[ICON_COUNT];    /* 32 px */
} Icons;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Load all icons from dir/<name>_16.png and dir/<name>_32.png. Missing files
 * are logged and drawn as nothing.
 */
PUB s32 icons_load(Icons *icons, const char *dir);
PUB void icons_unload(Icons *icons);

/*
 * The icon at px pixels, put into the atlas on first use.
 */
PUB const AtlasEntry *icons_get(Icons *icons, Atlas *atlas, u16 id, s32 px);

/*
 * File name of an icon ("close", ...).
 */
PUB const char *icons_name(u16 id);

#endif /* VASALL_STUDIO_ICONS_H */
