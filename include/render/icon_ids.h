#ifndef VASALL_STUDIO_ICON_IDS_H
#define VASALL_STUDIO_ICON_IDS_H

/*
 * All icons in one place. The name table in render/icons.c must have the
 * same order; files are assets/icons/<name>_16.png and <name>_32.png.
 */
typedef enum IconId {
        ICON_NONE = 0,
        ICON_CLOSE,
        ICON_DOT,
        ICON_CHEVRON_RIGHT,
        ICON_CHEVRON_DOWN,
        ICON_SPLIT_HORIZONTAL,
        ICON_SPLIT_VERTICAL,
        ICON_FILES,
        ICON_SEARCH,
        ICON_GEAR,
        ICON_WARNING,
        ICON_ERROR,
        ICON_INFO,
        ICON_TERMINAL,
        ICON_MORE,
        ICON_LAYOUT_SIDEBAR,
        ICON_LAYOUT_PANEL,
        ICON_ADD,
        ICON_CUBE,
        ICON_CHECK,
        ICON_FOLDER,
        ICON_FILE,
        ICON_COUNT
} IconId;

/* Logical size icons are drawn at */
#define ICON_SIZE 16

#endif /* VASALL_STUDIO_ICON_IDS_H */
