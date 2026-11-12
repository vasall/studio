#include <workbench/gallery_view.h>

#include <render/icon_ids.h>
#include <ui/widgets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIST_ROWS 10000
#define ROW_H     22.0f

/* A small project tree, in pre-order */
typedef struct DemoNode {
        s32 depth;
        const char *name;
        s32 folder;
} DemoNode;

LOC const DemoNode s_tree[] = {
        {0, "parts", 1},
        {1, "bracket.vpart", 0},
        {1, "base_plate.vpart", 0},
        {1, "servo_mount.vpart", 0},
        {1, "fasteners", 1},
        {2, "m3_screw.vpart", 0},
        {2, "m3_nut.vpart", 0},
        {2, "washer.vpart", 0},
        {1, "wheel.vpart", 0},
        {0, "assemblies", 1},
        {1, "chassis.vasm", 0},
        {1, "arm.vasm", 0},
        {0, "scripts", 1},
        {1, "drive.py", 0},
        {1, "sensors.py", 0},
        {1, "lib", 1},
        {2, "pid.py", 0},
        {2, "filters.py", 0},
        {0, "project.vasall", 0}
};

#define TREE_COUNT ((s32)(sizeof(s_tree) / sizeof(s_tree[0])))

typedef struct Gallery {
        s32 clicks;
        s32 grid;
        s32 snap;
        s32 shadows;
        s32 busy;
        s32 units;
        char name[64];
        char search[128];
        char submitted[64];
        f32 opacity;
        f32 progress;
        s32 material;
        s32 tree_open[TREE_COUNT];
        s32 tree_selected;
        s32 list_selected;
} Gallery;

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_draw(View *view, UI *ui, UIBox *area);
LOC u16 s_icon(View *view);
LOC void s_destroy(View *view);

LOC UIBox *s_section(UI *ui, const char *title);
LOC UIBox *s_row(UI *ui);
LOC void s_buttons(Gallery *g, UI *ui);
LOC void s_toggles(Gallery *g, UI *ui);
LOC void s_text(Gallery *g, UI *ui);
LOC void s_values(Gallery *g, UI *ui);
LOC void s_badges(UI *ui);
LOC void s_tree_and_list(Gallery *g, UI *ui);

const ViewClass gallery_view_class = {
        "gallery",
        s_draw,
        s_icon,
        NULL,
        s_destroy
};

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB ViewHandle gallery_view_create(Views *views)
{
        Gallery *g = calloc(1, sizeof(Gallery));

        if(g) {
                g->grid = 1;
                g->snap = 1;
                g->opacity = 0.8f;
                g->progress = 0.35f;
                g->material = 0;
                g->tree_open[0] = 1;
                g->tree_selected = 1;
                g->list_selected = -1;
        }
        return view_create(views, &gallery_view_class, "UI Gallery", g);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC void s_draw(View *view, UI *ui, UIBox *area)
{
        Gallery *g = view->data;

        (void)area;
        if(!g) {
                ui_label(ui, "Out of memory", "gallery-text");
                return;
        }

        ui_scroll_begin(ui, "gallery", "gallery");
        ui_label(ui, "UI Gallery", "gallery-title");
        ui_label(ui, "Every widget in one place. Tab and Shift+Tab move the "
                        "keyboard focus, Esc drops it.", "gallery-hint");

        s_buttons(g, ui);
        s_toggles(g, ui);
        s_text(g, ui);
        s_values(g, ui);
        s_badges(ui);
        s_tree_and_list(g, ui);
        ui_scroll_end(ui);
}

LOC u16 s_icon(View *view)
{
        (void)view;
        return ICON_LAYOUT_SIDEBAR;
}

LOC void s_destroy(View *view)
{
        free(view->data);
        view->data = NULL;
}

/* A titled column; close it with ui_box_end */
LOC UIBox *s_section(UI *ui, const char *title)
{
        UIBox *box = ui_box_begin(ui, title, "gallery-section", 0);

        ui_label(ui, title, "gallery-heading");
        return box;
}

/* A wrapping row; close it with ui_box_end */
LOC UIBox *s_row(UI *ui)
{
        return ui_box_begin(ui, NULL, "gallery-row", 0);
}

LOC void s_buttons(Gallery *g, UI *ui)
{
        s_section(ui, "Buttons and notifications");
        s_row(ui);
        if(ui_button(ui, "Click me", NULL).clicked) g->clicks++;
        if(ui_button(ui, "Info toast", NULL).clicked)
                ui_toast(ui, UI_TOAST_INFO, "Part \"bracket\" rebuilt in 12 ms");
        if(ui_button(ui, "Warning toast", NULL).clicked)
                ui_toast(ui, UI_TOAST_WARNING, "Wall thinner than 0.8 mm");
        if(ui_button(ui, "Error toast", NULL).clicked)
                ui_toast(ui, UI_TOAST_ERROR, "Sandbox connection lost");
        ui_icon_button(ui, "add", ICON_ADD, "Add", NULL);
        ui_icon_button(ui, "search", ICON_SEARCH, "Search", NULL);
        ui_icon_button(ui, "settings", ICON_GEAR, "Settings", NULL);
        ui_label(ui, arena_printf(&ui->arena, "Clicked %d times", g->clicks),
                        "gallery-text");
        ui_box_end(ui); /* gallery-row */
        ui_box_end(ui); /* gallery-section */
}

LOC void s_toggles(Gallery *g, UI *ui)
{
        s_section(ui, "Checkboxes and radio buttons");
        s_row(ui);
        ui_checkbox(ui, "Show grid", &g->grid, NULL);
        ui_checkbox(ui, "Snap to grid", &g->snap, NULL);
        ui_checkbox(ui, "Shadows", &g->shadows, NULL);
        ui_box_end(ui); /* gallery-row */
        s_row(ui);
        ui_radio(ui, "Millimeters", &g->units, 0, NULL);
        ui_radio(ui, "Inches", &g->units, 1, NULL);
        ui_radio(ui, "Meters", &g->units, 2, NULL);
        ui_box_end(ui); /* gallery-row */
        ui_box_end(ui); /* gallery-section */
}

LOC void s_text(Gallery *g, UI *ui)
{
        UITextFieldResult r;

        s_section(ui, "Text fields");
        s_row(ui);
        r = ui_text_field(ui, "name", g->name, sizeof(g->name), "Part name",
                        NULL);
        if(r.submitted) {
                snprintf(g->submitted, sizeof(g->submitted), "%s", g->name);
                ui_toast(ui, UI_TOAST_INFO, arena_printf(&ui->arena,
                                        "Renamed to \"%s\"", g->name));
        }
        ui_text_field(ui, "search", g->search, sizeof(g->search),
                        "Search files", NULL);
        ui_box_end(ui); /* gallery-row */
        ui_label(ui, g->submitted[0] ? arena_printf(&ui->arena,
                                "Last submitted: %s", g->submitted) :
                        "Press Enter in the first field to submit",
                        "gallery-hint");
        ui_box_end(ui); /* gallery-section */
}

LOC void s_values(Gallery *g, UI *ui)
{
        static const char *const materials[] = {
                "PLA", "PETG", "ABS", "Aluminium 6061", "Steel S235"
        };

        s_section(ui, "Sliders, progress and dropdowns");
        s_row(ui);
        ui_slider(ui, "opacity", &g->opacity, 0.0f, 1.0f, NULL);
        ui_label(ui, arena_printf(&ui->arena, "Opacity %3.0f %%",
                                g->opacity * 100.0f), "gallery-text");
        ui_box_end(ui); /* gallery-row */
        s_row(ui);
        ui_slider(ui, "progress-value", &g->progress, 0.0f, 1.0f, NULL);
        ui_progress(ui, "progress", g->progress, NULL);
        ui_box_end(ui); /* gallery-row */
        s_row(ui);
        /* It draws every frame while shown, so it's off by default */
        ui_progress(ui, "busy", g->busy ? -1.0f : 0.0f, NULL);
        ui_checkbox(ui, "Busy (unknown duration)", &g->busy, NULL);
        ui_box_end(ui); /* gallery-row */
        s_row(ui);
        if(ui_dropdown(ui, "material", &g->material, materials,
                                (s32)(sizeof(materials) / sizeof(materials[0])),
                                NULL)) {
                ui_toast(ui, UI_TOAST_INFO, arena_printf(&ui->arena,
                                        "Material set to %s",
                                        materials[g->material]));
        }
        ui_label(ui, "Material", "gallery-text");
        ui_box_end(ui); /* gallery-row */
        ui_box_end(ui); /* gallery-section */
}

LOC void s_badges(UI *ui)
{
        s_section(ui, "Badges");
        s_row(ui);
        ui_badge(ui, "3", NULL);
        ui_badge(ui, "New", NULL);
        ui_badge(ui, "Draft", "badge.gray");
        ui_badge(ui, "2 warnings", "badge.warning");
        ui_badge(ui, "1 error", "badge.error");
        ui_box_end(ui); /* gallery-row */
        ui_box_end(ui); /* gallery-section */
}

LOC void s_tree_and_list(Gallery *g, UI *ui)
{
        UITreeRowResult r;
        UIList list;
        UIBox *row;
        s32 visible[TREE_COUNT];
        s32 n = 0;
        s32 hide_below = -1;
        s32 i;
        s32 k;

        /* Rows under a collapsed folder are hidden */
        for(i = 0; i < TREE_COUNT; i++) {
                if(hide_below >= 0 && s_tree[i].depth > hide_below) continue;
                hide_below = -1;
                visible[n++] = i;
                if(s_tree[i].folder && !g->tree_open[i]){
                        hide_below = s_tree[i].depth;
                }
        }

        s_section(ui, "Tree and virtualized list");
        s_row(ui);

        list = ui_list_begin(ui, "tree", n, ROW_H, "gallery-tree");
        for(k = list.first; k < list.last; k++) {
                i = visible[k];
                ui_push_id(ui, (u64)i);
                r = ui_tree_row(ui, "row", s_tree[i].name,
                                s_tree[i].folder ? ICON_FOLDER : ICON_FILE,
                                s_tree[i].depth, s_tree[i].folder,
                                &g->tree_open[i], g->tree_selected == i, NULL);
                if(r.sig.pressed) g->tree_selected = i;
                ui_pop_id(ui);
        }
        ui_list_end(ui, &list);

        list = ui_list_begin(ui, "rows", LIST_ROWS, ROW_H, "gallery-tree");
        for(i = list.first; i < list.last; i++) {
                ui_push_id(ui, (u64)i);
                r = ui_tree_row(ui, "row", arena_printf(&ui->arena,
                                        "Row %05d", i), ICON_DOT, 0, 0, NULL,
                                g->list_selected == i, NULL);
                if(r.sig.pressed) g->list_selected = i;
                ui_pop_id(ui);
        }
        ui_list_end(ui, &list);

        row = ui_box_begin(ui, NULL, "gallery-section", 0);
        row->style.width = ui_fit();
        ui_label(ui, arena_printf(&ui->arena, "%d rows, %d built",
                                LIST_ROWS, list.last - list.first),
                        "gallery-text");
        if(ui_button(ui, "Jump to row 5000", NULL).clicked) {
                g->list_selected = 5000;
                ui_list_scroll_to(ui, &list, 5000);
        }
        ui_box_end(ui); /* gallery-section */

        ui_box_end(ui); /* gallery-row */
        ui_box_end(ui); /* gallery-section */
}
