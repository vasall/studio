#include <ui/style.h>

#include <string.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UiSize ui_fit(void)
{
        UiSize s;

        s.kind = UI_SIZE_FIT;
        s.value = 0.0f;
        s.min = 0.0f;
        s.max = 0.0f;
        return s;
}

PUB UiSize ui_px(f32 px)
{
        UiSize s = ui_fit();

        s.kind = UI_SIZE_PIXELS;
        s.value = px;
        return s;
}

PUB UiSize ui_pct(f32 pct)
{
        UiSize s = ui_fit();

        s.kind = UI_SIZE_PERCENT;
        s.value = pct;
        return s;
}

PUB UiSize ui_grow(f32 weight)
{
        UiSize s = ui_fit();

        s.kind = UI_SIZE_GROW;
        s.value = weight;
        return s;
}

PUB UiColor ui_rgba(u8 r, u8 g, u8 b, u8 a)
{
        UiColor c;

        c.r = r;
        c.g = g;
        c.b = b;
        c.a = a;
        return c;
}

PUB UiColor ui_hex(u32 rrggbbaa)
{
        return ui_rgba((u8)(rrggbbaa >> 24), (u8)(rrggbbaa >> 16),
                        (u8)(rrggbbaa >> 8), (u8)rrggbbaa);
}

PUB UiColor ui_color_none(void)
{
        return ui_rgba(0, 0, 0, 0);
}

PUB UiRect ui_rect(f32 x, f32 y, f32 w, f32 h)
{
        UiRect r;

        r.x = x;
        r.y = y;
        r.w = w;
        r.h = h;
        return r;
}

PUB UiRect ui_rect_intersect(UiRect a, UiRect b)
{
        f32 x0 = a.x > b.x ? a.x : b.x;
        f32 y0 = a.y > b.y ? a.y : b.y;
        f32 x1 = a.x + a.w < b.x + b.w ? a.x + a.w : b.x + b.w;
        f32 y1 = a.y + a.h < b.y + b.h ? a.y + a.h : b.y + b.h;

        if(x1 < x0) x1 = x0;
        if(y1 < y0) y1 = y0;
        return ui_rect(x0, y0, x1 - x0, y1 - y0);
}

PUB s32 ui_rect_contains(UiRect r, f32 x, f32 y)
{
        return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

PUB void ui_sides(f32 out[4], f32 all)
{
        ui_sides4(out, all, all, all, all);
}

PUB void ui_sides4(f32 out[4], f32 l, f32 t, f32 r, f32 b)
{
        out[UI_LEFT] = l;
        out[UI_TOP] = t;
        out[UI_RIGHT] = r;
        out[UI_BOTTOM] = b;
}

PUB void ui_radius(f32 out[4], f32 all)
{
        ui_radius4(out, all, all, all, all);
}

PUB void ui_radius4(f32 out[4], f32 tl, f32 tr, f32 br, f32 bl)
{
        out[UI_TOP_LEFT] = tl;
        out[UI_TOP_RIGHT] = tr;
        out[UI_BOTTOM_RIGHT] = br;
        out[UI_BOTTOM_LEFT] = bl;
}

PUB UiStyle ui_style_default(void)
{
        UiStyle s;

        memset(&s, 0, sizeof(s));
        s.width = ui_fit();
        s.height = ui_fit();
        s.layout = UI_LAYOUT_ROW;
        s.font = UI_FONT_UI;
        s.font_size = 13.0f;
        s.text_color = ui_rgba(204, 204, 204, 255);
        return s;
}

PUB void ui_style_apply(UiStyle *dst, const UiStyle *src, u32 mask)
{
        if(mask & UI_FIELD_WIDTH) dst->width = src->width;
        if(mask & UI_FIELD_HEIGHT) dst->height = src->height;
        if(mask & UI_FIELD_INFILL) dst->infill = src->infill;
        if(mask & UI_FIELD_BORDER_COLOR) dst->border_color = src->border_color;
        if(mask & UI_FIELD_BORDER_WIDTH) dst->border_width = src->border_width;
        if(mask & UI_FIELD_CORNER_RADIUS)
                memcpy(dst->corner_radius, src->corner_radius,
                                sizeof(dst->corner_radius));
        if(mask & UI_FIELD_MARGIN)
                memcpy(dst->margin, src->margin, sizeof(dst->margin));
        if(mask & UI_FIELD_PADDING)
                memcpy(dst->padding, src->padding, sizeof(dst->padding));
        if(mask & UI_FIELD_GAP) dst->gap = src->gap;
        if(mask & UI_FIELD_ALIGN_X) dst->align_x = src->align_x;
        if(mask & UI_FIELD_ALIGN_Y) dst->align_y = src->align_y;
        if(mask & UI_FIELD_LAYOUT) dst->layout = src->layout;
        if(mask & UI_FIELD_WRAP) dst->wrap = src->wrap;
        if(mask & UI_FIELD_OVERFLOW) dst->overflow = src->overflow;
        if(mask & UI_FIELD_FONT) dst->font = src->font;
        if(mask & UI_FIELD_FONT_SIZE) dst->font_size = src->font_size;
        if(mask & UI_FIELD_TEXT_COLOR) dst->text_color = src->text_color;
}
