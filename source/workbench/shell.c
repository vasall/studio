#include <workbench/shell.h>

#include <devkit/log/log.h>

#include <render/icon_ids.h>
#include <ui/debug.h>
#include <ui/widgets.h>
#include <workbench/dummy_view.h>
#include <workbench/gallery_view.h>

#include <stdio.h>
#include <string.h>

#define SIDEBAR_MIN 170.0f
#define PANEL_MIN   90.0f

enum {
        MENU_FILE = 0,
        MENU_EDIT,
        MENU_VIEW,
        MENU_HELP,
        MENU_COUNT
};

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC ViewHandle s_new_dummy(Workbench *wb);
LOC ViewHandle s_clone(Dock *dock, ViewHandle view, void *user);
LOC void s_shortcuts(Workbench *wb, UI *ui);
LOC void s_menubar(Workbench *wb, UI *ui);
LOC void s_menus(Workbench *wb, UI *ui);
LOC void s_activitybar(Workbench *wb, UI *ui);
LOC void s_sidebar(Workbench *wb, UI *ui);
LOC void s_panel(Workbench *wb, UI *ui);
LOC void s_statusbar(Workbench *wb, UI *ui);
LOC void s_placeholder(UI *ui, const char *text);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 workbench_init(Workbench *wb, AppWindow *win)
{
        static const char *menu_names[MENU_COUNT] = {
                "menu-file", "menu-edit", "menu-view", "menu-help"
        };
        DockNode *right;
        s32 i;

        memset(wb, 0, sizeof(*wb));
        wb->win = win;
        wb->sidebar_visible = 1;
        wb->sidebar_width = 260.0f;
        wb->panel_visible = 1;
        wb->panel_height = 220.0f;
        wb->panel_tab = SHELL_PANEL_OUTPUT;

        for(i = 0; i < MENU_COUNT; i++) {
                wb->menu_keys[i] = hash_str(menu_names[i], HASH_SEED);
        }

        if(views_init(&wb->views) < 0) return -1;
        if(dock_init(&wb->dock, &wb->views, s_clone, wb) < 0) return -1;

        /* Three tabs on the left, two in a group on the right */
        for(i = 0; i < 3; i++) {
                dock_open(&wb->dock, NULL, s_new_dummy(wb));
        }

        right = dock_split(&wb->dock, wb->dock.active_group, DOCK_ZONE_RIGHT,
                        s_new_dummy(wb));

        dock_open(&wb->dock, right, s_new_dummy(wb));
        dock_open(&wb->dock, right, gallery_view_create(&wb->views));
        return 0;
}

PUB void workbench_deinit(Workbench *wb)
{
        dock_deinit(&wb->dock);
        views_deinit(&wb->views);
}

PUB void workbench_frame(AppWindow *win, void *user)
{
        Workbench *wb = user;
        UI *ui = &win->ui;
        UIBox *root;
        UIBox *main_row;
        UIBox *center;
        UIBox *editor;
        f32 delta;
        char info[96];

        root = ui_box_begin(ui, "workbench", "workbench", 0);
        root->style.width = ui_grow(1.0f);
        root->style.height = ui_grow(1.0f);
        root->style.layout = UI_LAYOUT_COLUMN;

        s_menubar(wb, ui);

        main_row = ui_box_begin(ui, "main", NULL, 0);
        main_row->style.width = ui_grow(1.0f);
        main_row->style.height = ui_grow(1.0f);

        s_activitybar(wb, ui);

        if(wb->sidebar_visible) {
                s_sidebar(wb, ui);
                delta = ui_splitter(ui, "sidebar-sash", UI_AXIS_X, NULL);
                wb->sidebar_width += delta;
                if(wb->sidebar_width > win->width * 0.6f)
                        wb->sidebar_width = win->width * 0.6f;
                if(wb->sidebar_width < SIDEBAR_MIN)
                        wb->sidebar_width = SIDEBAR_MIN;
        }

        center = ui_box_begin(ui, "center", NULL, 0);
        center->style.width = ui_grow(1.0f);
        center->style.height = ui_grow(1.0f);
        center->style.layout = UI_LAYOUT_COLUMN;

        editor = ui_box_begin(ui, "editor-area", "editor-area", 0);
        editor->style.width = ui_grow(1.0f);
        editor->style.height = ui_grow(1.0f);
        editor->style.width.min = 120.0f;
        editor->style.height.min = 80.0f;
        dock_build(&wb->dock, ui);
        ui_box_end(ui); /* editor-area */

        if(wb->panel_visible) {
                /* Dragging up makes the panel taller */
                delta = ui_splitter(ui, "panel-sash", UI_AXIS_Y, NULL);
                wb->panel_height -= delta;
                if(wb->panel_height > win->height * 0.75f)
                        wb->panel_height = win->height * 0.75f;
                if(wb->panel_height < PANEL_MIN)
                        wb->panel_height = PANEL_MIN;
                s_panel(wb, ui);
        }
        ui_box_end(ui); /* center */

        ui_box_end(ui); /* main */

        s_statusbar(wb, ui);
        ui_box_end(ui); /* workbench */

        s_menus(wb, ui);
        ui_toasts(ui, 22.0f);

        /* After building, so focused widgets (text fields) see keys first */
        s_shortcuts(wb, ui);

        snprintf(info, sizeof(info), "frame %.2f ms  draw calls %d  quads %d",
                        win->frame_ms, win->app->draw.draw_calls,
                        win->app->draw.quads);
        ui_debug_panel(ui, info);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC ViewHandle s_new_dummy(Workbench *wb)
{
        char title[32];

        snprintf(title, sizeof(title), "Dummy %u", ++wb->dummy_count);
        return dummy_view_create(&wb->views, title);
}

/* Split editor: a fresh view of the same kind */
LOC ViewHandle s_clone(Dock *dock, ViewHandle view, void *user)
{
        Workbench *wb = user;
        View *v = view_get(dock->views, view);

        if(!v) return view;
        if(v->cls == &gallery_view_class)
                return gallery_view_create(&wb->views);
        return dummy_view_create(&wb->views, v->title);
}

/* Keyboard shortcuts, until the command system (M7) exists */
LOC void s_shortcuts(Workbench *wb, UI *ui)
{
        UIInput *in = ui->input;
        UIEvent *ev;
        s32 i;

        for(i = 0; i < in->event_count; i++) {
                ev = &in->events[i];

                if(ui_event_is_key(ev, 'n', UI_SCAN_NONE, UI_MOD_CTRL)) {
                        dock_open(&wb->dock, NULL, s_new_dummy(wb));
                } else if(ui_event_is_key(ev, 'w', UI_SCAN_NONE,
                                        UI_MOD_CTRL)) {
                        dock_close_active(&wb->dock);
                } else if(ui_event_is_key(ev, 'b', UI_SCAN_NONE,
                                        UI_MOD_CTRL)) {
                        wb->sidebar_visible = !wb->sidebar_visible;
                } else if(ui_event_is_key(ev, 'j', UI_SCAN_NONE,
                                        UI_MOD_CTRL)) {
                        wb->panel_visible = !wb->panel_visible;
                } else if(ui_event_is_key(ev, '\\', UI_SCAN_BACKSLASH,
                                        UI_MOD_CTRL)) {
                        dock_split_active(&wb->dock, DOCK_ZONE_RIGHT);
                } else if(ui_event_is_key(ev, '\\', UI_SCAN_BACKSLASH,
                                        UI_MOD_CTRL | UI_MOD_SHIFT)) {
                        dock_split_active(&wb->dock, DOCK_ZONE_BOTTOM);
                } else if(ui_event_is_key(ev, UI_KEY_F12, UI_SCAN_NONE, 0)) {
                        ui->debug = !ui->debug;
                } else {
                        continue;
                }
                ev->consumed = 1;
                ui->changed = 1;
        }
}

LOC void s_menubar(Workbench *wb, UI *ui)
{
        static const char *labels[MENU_COUNT] = {
                "File##menubar",
                "Edit##menubar",
                "View##menubar",
                "Help##menubar"
        };
        UIBox *bar;
        UIBox *item;
        UISignal sig;
        s32 open = -1;
        s32 i;

        for(i = 0; i < MENU_COUNT; i++) {
                if(ui_popup_is_open(ui, wb->menu_keys[i])) open = i;
        }

        bar = ui_box_begin(ui, "menubar", "menubar", 0);
        bar->style.width = ui_grow(1.0f);

        ui_icon(ui, ICON_CUBE, "menubar-logo");
        for(i = 0; i < MENU_COUNT; i++) {
                item = ui_box(ui, labels[i], "menubar-item",
                                UI_BOX_CLICKABLE |
                                (open == i ? UI_BOX_SELECTED : 0));
                ui_box_set_text(ui, item, labels[i]);
                sig = ui_signal(ui, item);

                /* Click opens; while one is open, hovering switches */
                if((sig.pressed && open != i) ||
                   (sig.hovering && open >= 0 && open != i)) {
                        if(item->state) {
                                ui_popup_open(ui, wb->menu_keys[i],
                                                item->state->rect.x,
                                                item->state->rect.y +
                                                item->state->rect.h);
                        }
                } else if(sig.pressed && open == i) {
                        ui_popup_close(ui);
                }
        }

        ui_spacer(ui, ui_grow(1.0f), ui_px(0.0f));
        ui_label(ui, "Vasall Studio", "menubar-title");
        ui_spacer(ui, ui_grow(1.0f), ui_px(0.0f));
        ui_box_end(ui); /* menubar */
}

LOC void s_menus(Workbench *wb, UI *ui)
{
        if(ui_menu_begin(ui, wb->menu_keys[MENU_FILE])) {
                if(ui_menu_item(ui, "New Dummy Tab", "Ctrl+N"))
                        dock_open(&wb->dock, NULL, s_new_dummy(wb));
                if(ui_menu_item(ui, "Close Editor", "Ctrl+W"))
                        dock_close_active(&wb->dock);
                ui_menu_separator(ui);
                if(ui_menu_item(ui, "Exit", NULL))
                        wb->win->app->running = 0;
                ui_menu_end(ui);
        }

        if(ui_menu_begin(ui, wb->menu_keys[MENU_EDIT])) {
                ui_menu_item(ui, "Undo", "Ctrl+Z");
                ui_menu_item(ui, "Redo", "Ctrl+Y");
                ui_menu_separator(ui);
                ui_menu_item(ui, "Cut", "Ctrl+X");
                ui_menu_item(ui, "Copy", "Ctrl+C");
                ui_menu_item(ui, "Paste", "Ctrl+V");
                ui_menu_end(ui);
        }

        if(ui_menu_begin(ui, wb->menu_keys[MENU_VIEW])) {
                if(ui_menu_item(ui, "Toggle Side Bar", "Ctrl+B"))
                        wb->sidebar_visible = !wb->sidebar_visible;
                if(ui_menu_item(ui, "Toggle Panel", "Ctrl+J"))
                        wb->panel_visible = !wb->panel_visible;
                ui_menu_separator(ui);
                if(ui_menu_item(ui, "Split Editor Right", "Ctrl+\\"))
                        dock_split_active(&wb->dock, DOCK_ZONE_RIGHT);
                if(ui_menu_item(ui, "Split Editor Down", "Ctrl+Shift+\\"))
                        dock_split_active(&wb->dock, DOCK_ZONE_BOTTOM);
                ui_menu_separator(ui);
                if(ui_menu_item(ui, "Open UI Gallery", NULL))
                        dock_open(&wb->dock, NULL,
                                        gallery_view_create(&wb->views));
                if(ui_menu_item(ui, "Toggle Debug Overlay", "F12"))
                        ui->debug = !ui->debug;
                ui_menu_end(ui);
        }

        if(ui_menu_begin(ui, wb->menu_keys[MENU_HELP])) {
                ui_menu_item(ui, "About Vasall Studio", NULL);
                ui_menu_end(ui);
        }
}

LOC void s_activitybar(Workbench *wb, UI *ui)
{
        static const u16 icons[SHELL_ACTIVITY_COUNT] = {
                ICON_FILES, ICON_SEARCH
        };
        static const char *tips[SHELL_ACTIVITY_COUNT] = {
                "Explorer (Ctrl+B)", "Search"
        };
        UIBox *bar;
        UIBox *item;
        UISignal sig;
        char id[16];
        s32 selected;
        s32 i;

        bar = ui_box_begin(ui, "activitybar", "activitybar", 0);
        bar->style.layout = UI_LAYOUT_COLUMN;
        bar->style.height = ui_grow(1.0f);

        for(i = 0; i < SHELL_ACTIVITY_COUNT; i++) {
                selected = wb->sidebar_visible && wb->activity == i;
                snprintf(id, sizeof(id), "activity-%d", i);
                item = ui_box(ui, id, "activity-item", UI_BOX_CLICKABLE |
                                (selected ? UI_BOX_SELECTED : 0));
                item->icon = icons[i];
                item->cursor = UI_CURSOR_POINTER;
                ui_tooltip(ui, item, tips[i]);
                sig = ui_signal(ui, item);

                /* Clicking the shown activity hides the side bar */
                if(sig.clicked) {
                        if(selected) {
                                wb->sidebar_visible = 0;
                        } else {
                                wb->activity = i;
                                wb->sidebar_visible = 1;
                        }
                }
        }

        ui_spacer(ui, ui_px(0.0f), ui_grow(1.0f));
        item = ui_box(ui, "activity-settings", "activity-item",
                        UI_BOX_CLICKABLE);
        item->icon = ICON_GEAR;
        ui_tooltip(ui, item, "Settings");
        ui_signal(ui, item);
        ui_box_end(ui); /* activitybar */
}

LOC void s_sidebar(Workbench *wb, UI *ui)
{
        UIBox *side;
        UIBox *header;

        side = ui_box_begin(ui, "sidebar", "sidebar", 0);
        side->style.layout = UI_LAYOUT_COLUMN;
        side->style.width = ui_px(wb->sidebar_width);
        side->style.height = ui_grow(1.0f);

        header = ui_box_begin(ui, "sidebar-header", "sidebar-header", 0);
        header->style.width = ui_grow(1.0f);
        ui_label(ui, wb->activity == SHELL_EXPLORER ? "EXPLORER" : "SEARCH",
                        "sidebar-title");
        ui_spacer(ui, ui_grow(1.0f), ui_px(0.0f));
        ui_icon_button(ui, "sidebar-more", ICON_MORE, "More Actions...", NULL);
        ui_box_end(ui); /* sidebar-header */

        s_placeholder(ui, wb->activity == SHELL_EXPLORER ?
                        "No folder opened" : "Search is not available yet");
        ui_box_end(ui); /* sidebar */
}

LOC void s_panel(Workbench *wb, UI *ui)
{
        static const char *tabs[SHELL_PANEL_COUNT] = {
                "PROBLEMS##panel", "OUTPUT##panel", "TERMINAL##panel"
        };
        static const char *empty[SHELL_PANEL_COUNT] = {
                "No problems have been detected.",
                "Output will appear here.",
                "The terminal is not available yet."
        };
        UIBox *panel;
        UIBox *header;
        UIBox *tab;
        s32 i;

        panel = ui_box_begin(ui, "panel", "panel", 0);
        panel->style.layout = UI_LAYOUT_COLUMN;
        panel->style.width = ui_grow(1.0f);
        panel->style.height = ui_px(wb->panel_height);

        header = ui_box_begin(ui, "panel-header", "panel-header", 0);
        header->style.width = ui_grow(1.0f);
        for(i = 0; i < SHELL_PANEL_COUNT; i++) {
                tab = ui_box(ui, tabs[i], "panel-tab", UI_BOX_CLICKABLE |
                                (wb->panel_tab == i ? UI_BOX_SELECTED : 0));
                ui_box_set_text(ui, tab, tabs[i]);
                tab->cursor = UI_CURSOR_POINTER;
                if(ui_signal(ui, tab).pressed) wb->panel_tab = i;
        }
        ui_spacer(ui, ui_grow(1.0f), ui_px(0.0f));
        if(ui_icon_button(ui, "panel-close", ICON_CLOSE, "Close Panel (Ctrl+J)",
                                NULL).clicked)
                wb->panel_visible = 0;
        ui_box_end(ui); /* panel-header */

        s_placeholder(ui, empty[wb->panel_tab]);
        ui_box_end(ui); /* panel */
}

LOC void s_statusbar(Workbench *wb, UI *ui)
{
        UIBox *bar;
        UIBox *item;
        View *v;

        bar = ui_box_begin(ui, "statusbar", "statusbar", 0);
        bar->style.width = ui_grow(1.0f);

        item = ui_box(ui, "status-problems", "status-item", UI_BOX_CLICKABLE);
        item->icon = ICON_ERROR;
        ui_box_set_text(ui, item, "0");
        ui_tooltip(ui, item, "No problems");
        if(ui_signal(ui, item).clicked) {
                wb->panel_visible = 1;
                wb->panel_tab = SHELL_PANEL_PROBLEMS;
        }
        item = ui_box(ui, "status-warnings", "status-item", UI_BOX_CLICKABLE);
        item->icon = ICON_WARNING;
        ui_box_set_text(ui, item, "0");
        ui_signal(ui, item);

        ui_spacer(ui, ui_grow(1.0f), ui_px(0.0f));

        v = view_get(&wb->views, dock_active_view(&wb->dock));
        if(v) ui_label(ui, v->title, "status-text");
        ui_label(ui, arena_printf(&ui->arena, "%d groups",
                                dock_group_count(&wb->dock)), "status-text");
        ui_label(ui, "UTF-8", "status-text");
        ui_label(ui, "Vasall Studio", "status-text");
        ui_box_end(ui); /* statusbar */
}

/* An empty area with a muted, centered note */
LOC void s_placeholder(UI *ui, const char *text)
{
        UIBox *body;

        body = ui_box_begin(ui, NULL, "placeholder", 0);
        body->style.width = ui_grow(1.0f);
        body->style.height = ui_grow(1.0f);
        ui_label(ui, text, "placeholder-text");
        ui_box_end(ui); /* placeholder */
}
