#include <workbench/dummy_view.h>

#include <render/icon_ids.h>
#include <ui/widgets.h>

#include <math.h>
#include <stdio.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_draw(View *view, UI *ui, UIBox *area);
LOC u16 s_icon(View *view);
LOC UIColor s_hue(u32 id);

const ViewClass dummy_view_class = {
        "dummy",
        s_draw,
        s_icon,
        NULL,
        NULL
};

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB ViewHandle dummy_view_create(Views *views, const char *title)
{
        return view_create(views, &dummy_view_class, title, NULL);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC void s_draw(View *view, UI *ui, UIBox *area)
{
        UIBox *stripe;
        UIBox *card;
        char buf[64];

        (void)area;

        stripe = ui_box(ui, NULL, "dummy-stripe", 0);
        stripe->style.infill = s_hue(view->id);

        ui_box_begin(ui, NULL, "dummy-center", 0);
        card = ui_box_begin(ui, NULL, "dummy-card", 0);
        card->style.border_color = s_hue(view->id);
        ui_label(ui, view->title, "dummy-title");
        snprintf(buf, sizeof(buf), "Dummy view #%u", view->id);
        ui_label(ui, buf, "dummy-text");
        ui_label(ui, "Drag the tab to move or split", "dummy-hint");
        ui_box_end(ui); /* dummy-card */
        ui_box_end(ui); /* dummy-center */
}

LOC u16 s_icon(View *view)
{
        (void)view;
        return ICON_CUBE;
}

/* A distinct, muted color per id (golden-angle hue steps) */
LOC UIColor s_hue(u32 id)
{
        f32 h = fmodf((f32)id * 137.508f, 360.0f) / 60.0f;
        f32 s = 0.55f;
        f32 v = 0.80f;
        f32 c = v * s;
        f32 x = c * (1.0f - fabsf(fmodf(h, 2.0f) - 1.0f));
        f32 m = v - c;
        f32 r = 0.0f;
        f32 g = 0.0f;
        f32 b = 0.0f;

        if(h < 1.0f) {
                r = c;
                g = x;
        } else if(h < 2.0f) {
                r = x;
                g = c;
        } else if(h < 3.0f) {
                g = c;
                b = x;
        } else if(h < 4.0f) {
                g = x;
                b = c;
        } else if(h < 5.0f) {
                r = x;
                b = c;
        } else {
                r = c;
                b = x;
        }

        return ui_rgba((u8)((r + m) * 255.0f), (u8)((g + m) * 255.0f),
                        (u8)((b + m) * 255.0f), 255);
}
