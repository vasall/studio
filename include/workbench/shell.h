#ifndef VASALL_STUDIO_SHELL_H
#define VASALL_STUDIO_SHELL_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <platform/app.h>
#include <workbench/dock.h>
#include <workbench/view.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef enum ShellActivity {
        SHELL_EXPLORER = 0,
        SHELL_SEARCH,
        SHELL_ACTIVITY_COUNT
} ShellActivity;

typedef enum ShellPanelTab {
        SHELL_PANEL_PROBLEMS = 0,
        SHELL_PANEL_OUTPUT,
        SHELL_PANEL_TERMINAL,
        SHELL_PANEL_COUNT
} ShellPanelTab;

/*
 * The workbench of one window: menu bar, activity bar, side bar, editor
 * area (dock), bottom panel and status bar.
 */
typedef struct Workbench {
        AppWindow *win;
        Views views;
        Dock dock;

        s32 activity;           /* ShellActivity */
        s32 sidebar_visible;
        f32 sidebar_width;
        s32 panel_visible;
        f32 panel_height;
        s32 panel_tab;          /* ShellPanelTab */

        u32 dummy_count;        /* For "Dummy N" titles */
        UIBoxKey menu_keys[4];  /* File, Edit, View, Help */
} Workbench;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Set up the workbench with a few dummy tabs in two groups.
 */
PUB s32 workbench_init(Workbench *wb, AppWindow *win);
PUB void workbench_deinit(Workbench *wb);

/*
 * Frame callback for app_window_open (user = Workbench).
 */
PUB void workbench_frame(AppWindow *win, void *user);

#endif /* VASALL_STUDIO_SHELL_H */
