#include <workbench/gallery_view.h>

#include <render/canvas_demo.h>
#include <render/scene.h>
#include <render/icon_ids.h>
#include <ui/widgets/canvas.h>
#include <ui/widgets.h>

#include <math.h>
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
        s32 animate;

        /* 3D viewport, made on first draw (needs the GL context) */
        s32 scene_state;        /* 0 not yet, 1 ready, -1 failed */
        Scene3D scene;
        SceneView view;
        SceneObject spinner;
        s32 spin;
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
LOC void s_canvas(Gallery *g, UI *ui);
LOC void s_viewport(Gallery *g, UI *ui);
LOC s32 s_scene_setup(Gallery *g);
LOC SceneMesh s_box_mesh(Scene3D *scene);
LOC SceneMesh s_cylinder_mesh(Scene3D *scene, s32 segments);
LOC FMatrix4 s_place(f32 x, f32 y, f32 z, f32 sx, f32 sy, f32 sz, f32 rz);
LOC void s_reset_view(Gallery *g);
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
        s_canvas(g, ui);
        s_viewport(g, ui);
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
        Gallery *g = view->data;

        if(g && g->scene_state == 1) {
                scene_view_deinit(&g->view);
                scene_deinit(&g->scene);
        }
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

LOC void s_viewport(Gallery *g, UI *ui)
{
        f32 angle;

        s_section(ui, "3D viewport");
        if(g->scene_state == 0) g->scene_state = s_scene_setup(g) < 0 ? -1 : 1;

        if(g->scene_state == 1) {
                g->scene.background = ui_theme_color(ui->theme, "background",
                                ui_hex(0x161616ff));
                if(g->spin) {
                        /* Moving objects every frame, as live simulation
                           data will */
                        angle = (f32)fmod(ui->time, 6.283185307);
                        scene_object_set_transform(&g->scene, g->spinner,
                                        s_place(0, 0, 66, 24, 24, 24, angle));
                        ui->changed = 1;
                }
                scene_viewport(ui, "viewport", &g->scene, &g->view, NULL);
        } else {
                ui_label(ui, "The 3D scene could not be created",
                                "gallery-text");
        }

        s_row(ui);
        ui_checkbox(ui, "Spin", &g->spin, NULL);
        if(ui_button(ui, "Reset view", NULL).clicked) s_reset_view(g);
        ui_label(ui, "Drag to orbit, Shift + drag or middle drag to pan, "
                        "wheel to zoom.", "gallery-hint");
        ui_box_end(ui); /* gallery-row */
        ui_box_end(ui); /* gallery-section */
}

/* A small assembly: a plate, a column on it and a cube on top */
LOC s32 s_scene_setup(Gallery *g)
{
        SceneMesh box;
        SceneMesh cyl;

        if(scene_init(&g->scene, 10.0f, 30) < 0) return -1;
        scene_view_init(&g->view);
        s_reset_view(g);

        box = s_box_mesh(&g->scene);
        cyl = s_cylinder_mesh(&g->scene, 48);

        scene_object_add(&g->scene, box, s_place(0, 0, 3, 120, 80, 6, 0),
                        ui_hex(0x8d8d8dff));
        scene_object_add(&g->scene, cyl, s_place(0, 0, 6, 14, 14, 48, 0),
                        ui_hex(0x4589ffff));
        g->spinner = scene_object_add(&g->scene, box,
                        s_place(0, 0, 66, 24, 24, 24, 0), ui_hex(0x42be65ff));
        return 0;
}

LOC void s_reset_view(Gallery *g)
{
        camera_init(&g->view.camera);
        camera_frame(&g->view.camera, fvec3(0.0f, 0.0f, 30.0f), 80.0f);
}

/* A unit cube around the origin; no normals, so it is shaded flat */
LOC SceneMesh s_box_mesh(Scene3D *scene)
{
        static const f32 pos[8 * 3] = {
                -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f
        };
        /* Counter-clockwise seen from outside */
        static const u32 idx[12 * 3] = {
                0, 2, 1,  0, 3, 2,      /* bottom */
                4, 5, 6,  4, 6, 7,      /* top */
                0, 1, 5,  0, 5, 4,      /* front (-Y) */
                2, 3, 7,  2, 7, 6,      /* back */
                1, 2, 6,  1, 6, 5,      /* right (+X) */
                3, 0, 4,  3, 4, 7       /* left */
        };
        SceneMeshData d;

        memset(&d, 0, sizeof(d));
        d.positions = pos;
        d.indices = idx;
        d.vertex_count = 8;
        d.index_count = 36;
        return scene_mesh_add(scene, &d);
}

/**
 * A cylinder of radius 0.5 from z = 0 to 1, with normals: smooth sides,
 * flat caps (the caps have their own vertices).
 */
LOC SceneMesh s_cylinder_mesh(Scene3D *scene, s32 segments)
{
        SceneMeshData d;
        SceneMesh mesh = { POOL_HANDLE_NONE };
        f32 *pos;
        f32 *nrm;
        u32 *idx;
        s32 nv = (segments + 1) * 2 + (segments + 2) * 2;
        s32 ni = segments * 6 + segments * 3 * 2;
        s32 v = 0;
        s32 k = 0;
        s32 cap;
        s32 i;
        s32 c;
        f32 a;

        pos = malloc((size_t)nv * 3 * sizeof(f32));
        nrm = malloc((size_t)nv * 3 * sizeof(f32));
        idx = malloc((size_t)ni * sizeof(u32));
        if(!pos || !nrm || !idx) {
                free(pos);
                free(nrm);
                free(idx);
                return mesh;
        }

#define VERT(px, py, pz, nx, ny, nz) do { \
                pos[v * 3] = (px); pos[v * 3 + 1] = (py); pos[v * 3 + 2] = (pz); \
                nrm[v * 3] = (nx); nrm[v * 3 + 1] = (ny); nrm[v * 3 + 2] = (nz); \
                v++; } while(0)

        /* Sides: a bottom and top vertex per step, normals point outwards */
        for(i = 0; i <= segments; i++) {
                a = (f32)i / (f32)segments * 6.2831853f;
                VERT(0.5f * cosf(a), 0.5f * sinf(a), 0.0f, cosf(a), sinf(a), 0);
                VERT(0.5f * cosf(a), 0.5f * sinf(a), 1.0f, cosf(a), sinf(a), 0);
        }
        for(i = 0; i < segments; i++) {
                u32 b = (u32)(i * 2);

                idx[k++] = b;
                idx[k++] = b + 2;
                idx[k++] = b + 3;
                idx[k++] = b;
                idx[k++] = b + 3;
                idx[k++] = b + 1;
        }

        /* Caps: a center and a ring each */
        for(cap = 0; cap < 2; cap++) {
                f32 z = (f32)cap;
                f32 nz = cap ? 1.0f : -1.0f;
                u32 center = (u32)v;

                VERT(0.0f, 0.0f, z, 0.0f, 0.0f, nz);
                for(i = 0; i <= segments; i++) {
                        a = (f32)i / (f32)segments * 6.2831853f;
                        VERT(0.5f * cosf(a), 0.5f * sinf(a), z, 0, 0, nz);
                }
                for(i = 0; i < segments; i++) {
                        c = (s32)center + 1 + i;
                        idx[k++] = center;
                        idx[k++] = (u32)(cap ? c : c + 1);
                        idx[k++] = (u32)(cap ? c + 1 : c);
                }
        }
#undef VERT

        memset(&d, 0, sizeof(d));
        d.positions = pos;
        d.normals = nrm;
        d.indices = idx;
        d.vertex_count = v;
        d.index_count = k;
        mesh = scene_mesh_add(scene, &d);
        free(pos);
        free(nrm);
        free(idx);
        return mesh;
}

/* Translate * rotate around Z * scale */
LOC FMatrix4 s_place(f32 x, f32 y, f32 z, f32 sx, f32 sy, f32 sz, f32 rz)
{
        FMatrix4 s = fmat4_identity();

        s.data[0] = sx;
        s.data[5] = sy;
        s.data[10] = sz;
        return fmat4_mul(fmat4_translation(fvec4(x, y, z, 1.0f)),
                        fmat4_mul(fmat4_rotation_z(rz), s));
}

LOC void s_canvas(Gallery *g, UI *ui)
{
        UICanvas *cv;
        CanvasDemoBox *gl_box;
        UIColor grid = ui_theme_color(ui->theme, "border-subtle-00",
                        ui_hex(0x393939ff));
        UIColor accent = ui_theme_color(ui->theme, "interactive",
                        ui_hex(0x4589ffff));
        UIColor text = ui_theme_color(ui->theme, "text-primary",
                        ui_hex(0xf4f4f4ff));
        f32 x;
        f32 y;
        f32 phase = 0.0f;

        s_section(ui, "Canvas");
        cv = ui_canvas(ui, "canvas", NULL);
        if(cv) {
                /* Grid lines, through the UI renderer */
                for(x = 40.0f; x < cv->width; x += 40.0f)
                        ui_canvas_rect(cv, ui_rect(x, 0, 1, cv->height), grid);
                for(y = 40.0f; y < cv->height; y += 40.0f)
                        ui_canvas_rect(cv, ui_rect(0, y, cv->width, 1), grid);

                /* A box through the UI renderer */
                ui_canvas_box(cv, ui_rect(24, 24, 120, 72),
                                ui_rgba(accent.r, accent.g, accent.b, 60),
                                accent, 1.0f, 6.0f);

                /* A box with own GL code; it moves while animating, which
                   redraws every frame like a live plot would */
                if(g->animate) {
                        phase = (f32)sin(ui->time * 2.0) * 0.5f + 0.5f;
                        ui->changed = 1;
                }
                gl_box = arena_alloc(&ui->arena, sizeof(CanvasDemoBox));
                if(gl_box) {
                        gl_box->x = 0.35f + phase * 0.4f;
                        gl_box->y = 0.3f;
                        gl_box->w = 0.2f;
                        gl_box->h = 0.4f;
                        gl_box->color = ui_hex(0x42be65ff);
                        ui_canvas_gl(cv, ui_rect(0, 0, cv->width, cv->height),
                                        canvas_demo_draw, gl_box);
                }

                /* Shapes after the GL part are drawn on top of it */
                if(cv->sig.hovering) {
                        ui_canvas_box(cv, ui_rect(cv->sig.mouse_x - 4.0f,
                                                cv->sig.mouse_y - 4.0f, 8, 8),
                                        text, text, 0.0f, 4.0f);
                }
        }
        s_row(ui);
        ui_checkbox(ui, "Animate", &g->animate, NULL);
        ui_label(ui, "Blue: UI shapes. Green: own GL code. The dot follows "
                        "the mouse.", "gallery-hint");
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
