#ifndef VASALL_STUDIO_UI_THEME_H
#define VASALL_STUDIO_UI_THEME_H

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/list.h>
#include <devkit/memory/hashmap.h>

#include <devkit/util/hash.h>
#include <ui/style.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

#define UI_THEME_NAME_LEN  48
#define UI_THEME_VALUE_LEN 64

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/* Interaction states a class can override fields for */
typedef enum UiState {
        UI_STATE_HOVER = 0,
        UI_STATE_PRESSED,
        UI_STATE_FOCUSED,
        UI_STATE_SELECTED,
        UI_STATE_DISABLED,
        UI_STATE_COUNT
} UiState;

/*
 * A named style class, e.g. "button" or "button.primary". Only the fields in
 * the masks are set; the rest comes from the parent class or the default.
 */
typedef struct UiThemeClass {
        char name[UI_THEME_NAME_LEN];
        UiStyle base;
        u32 base_mask;
        UiStyle state[UI_STATE_COUNT];
        u32 state_mask[UI_STATE_COUNT];
} UiThemeClass;

typedef struct UiThemeVar {
        char name[UI_THEME_NAME_LEN];
        char value[UI_THEME_VALUE_LEN];
} UiThemeVar;

typedef struct UiTheme {
        List classes;           /* UiThemeClass */
        HashMap class_index;    /* hash(name) -> index + 1 */
        List vars;              /* UiThemeVar */
} UiTheme;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

PUB s32 ui_theme_init(UiTheme *theme);
PUB void ui_theme_deinit(UiTheme *theme);

/*
 * Parse theme text and add its variables and classes. Lines look like:
 *
 *   # comment
 *   $accent              = #007accff
 *   button.height        = 22
 *   button.padding       = 0 8          (CSS order: t r b l / v h / all)
 *   button:hover.infill  = $accent
 *
 * Bad lines are logged and skipped.
 *
 * @returns The number of bad lines
 */
PUB s32 ui_theme_load_string(UiTheme *theme, const char *text);

/*
 * Load a theme file, see ui_theme_load_string.
 *
 * @returns The number of bad lines, or -1 if the file can't be read
 */
PUB s32 ui_theme_load_file(UiTheme *theme, const char *path);

/*
 * Find a class by its full name, or NULL.
 */
PUB const UiThemeClass *ui_theme_class(const UiTheme *theme, const char *name);

/*
 * Build the style for a class name. "a.b" applies "a", then "a.b". Then the
 * overrides of every state bit set in states (1 << UiState) are applied.
 * NULL or "" gives the default style.
 */
PUB UiStyle ui_theme_style(const UiTheme *theme, const char *name, u32 states);

/*
 * Get a variable as a color ("accent" for $accent), or fallback.
 */
PUB UiColor ui_theme_color(const UiTheme *theme, const char *name,
                UiColor fallback);

/*
 * Get a variable as a number, or fallback.
 */
PUB f32 ui_theme_number(const UiTheme *theme, const char *name, f32 fallback);

#endif /* VASALL_STUDIO_UI_THEME_H */
