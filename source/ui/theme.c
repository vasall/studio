#include <ui/theme.h>

#include <devkit/log/log.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_LEN 256

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC s32 s_parse_line(UITheme *theme, char *line);
LOC UIThemeClass *s_class_get_or_add(UITheme *theme, const char *name);
LOC const char *s_resolve_var(const UITheme *theme, const char *value);
LOC s32 s_parse_field(const UITheme *theme, UIStyle *style, u32 *mask,
                const char *field, const char *value);
LOC s32 s_parse_color(const UITheme *theme, const char *value, UIColor *out);
LOC s32 s_parse_size(const char *value, UISize *out);
LOC s32 s_parse_numbers(const char *value, f32 *out, s32 max);
LOC s32 s_parse_sides(const char *value, f32 out[4]);
LOC s32 s_parse_corners(const char *value, f32 out[4]);
LOC s32 s_parse_enum(const char *value, const char **names, s32 count);
LOC char *s_trim(char *str);
LOC void s_copy(char *dst, const char *src, s32 size);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 ui_theme_init(UITheme *theme)
{
        if(!theme) return -1;

        if(list_init(&theme->classes, sizeof(UIThemeClass), 32) < 0)
                return -1;
        if(list_init(&theme->vars, sizeof(UIThemeVar), 32) < 0) {
                list_deinit(&theme->classes);
                return -1;
        }
        if(hashmap_init(&theme->class_index, 32) < 0) {
                list_deinit(&theme->classes);
                list_deinit(&theme->vars);
                return -1;
        }
        return 0;
}

PUB void ui_theme_deinit(UITheme *theme)
{
        if(!theme) return;
        list_deinit(&theme->classes);
        list_deinit(&theme->vars);
        hashmap_deinit(&theme->class_index);
}

PUB s32 ui_theme_load_string(UITheme *theme, const char *text)
{
        char line[LINE_LEN];
        const char *p = text;
        s32 bad = 0;
        s32 lineno = 0;
        s32 len;

        if(!theme || !text) return 0;

        while(*p) {
                len = 0;
                while(p[len] && p[len] != '\n') len++;
                lineno++;

                if(len < LINE_LEN) {
                        memcpy(line, p, (size_t)len);
                        line[len] = '\0';
                        if(s_parse_line(theme, line) < 0) {
                                LOG_I(LOG_WARN, "theme: bad line %d", lineno);
                                bad++;
                        }
                } else {
                        LOG_I(LOG_WARN, "theme: line %d too long", lineno);
                        bad++;
                }

                p += len;
                if(*p == '\n') p++;
        }
        return bad;
}

PUB s32 ui_theme_load_file(UITheme *theme, const char *path)
{
        FILE *fp;
        long size;
        char *buf;
        s32 bad;

        fp = fopen(path, "rb");
        if(!fp) {
                LOG_S(LOG_ERROR, "theme: can't open %s", (char *)path);
                return -1;
        }

        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buf = malloc((size_t)size + 1);
        if(!buf) {
                fclose(fp);
                return -1;
        }
        size = (long)fread(buf, 1, (size_t)size, fp);
        buf[size] = '\0';
        fclose(fp);

        bad = ui_theme_load_string(theme, buf);
        free(buf);
        return bad;
}

PUB const UIThemeClass *ui_theme_class(const UITheme *theme, const char *name)
{
        u64 idx;

        if(!theme || !name || !*name) return NULL;
        if(!hashmap_get((HashMap *)&theme->class_index,
                                hash_str(name, HASH_SEED), &idx))
                return NULL;
        return list_get((List *)&theme->classes, (s32)idx - 1);
}

PUB UIStyle ui_theme_style(const UITheme *theme, const char *name, u32 states)
{
        UIStyle style = ui_style_default();
        char prefix[UI_THEME_NAME_LEN];
        const UIThemeClass *chain[8];
        s32 chain_len = 0;
        s32 i;
        s32 s;
        s32 len;

        if(!theme || !name || !*name) return style;

        /* Collect "a", "a.b", "a.b.c" */
        len = (s32)strlen(name);
        if(len >= UI_THEME_NAME_LEN) len = UI_THEME_NAME_LEN - 1;
        for(i = 0; i <= len && chain_len < 8; i++) {
                if(i == len || name[i] == '.') {
                        memcpy(prefix, name, (size_t)i);
                        prefix[i] = '\0';
                        chain[chain_len] = ui_theme_class(theme, prefix);
                        if(chain[chain_len]) chain_len++;
                }
        }

        /* Base fields of the whole chain first, then state overrides */
        for(i = 0; i < chain_len; i++)
                ui_style_apply(&style, &chain[i]->base, chain[i]->base_mask);

        for(s = 0; s < UI_STATE_COUNT; s++) {
                if(!(states & (1u << s))) continue;
                for(i = 0; i < chain_len; i++)
                        ui_style_apply(&style, &chain[i]->state[s],
                                        chain[i]->state_mask[s]);
        }
        return style;
}

PUB UIColor ui_theme_color(const UITheme *theme, const char *name,
                UIColor fallback)
{
        char ref[UI_THEME_NAME_LEN + 1];
        UIColor c;

        if(!theme || !name) return fallback;

        ref[0] = '$';
        s_copy(ref + 1, name, UI_THEME_NAME_LEN);
        if(s_parse_color(theme, ref, &c) < 0) return fallback;
        return c;
}

PUB f32 ui_theme_number(const UITheme *theme, const char *name, f32 fallback)
{
        char ref[UI_THEME_NAME_LEN + 1];
        const char *value;
        f32 n;

        if(!theme || !name) return fallback;

        ref[0] = '$';
        s_copy(ref + 1, name, UI_THEME_NAME_LEN);
        value = s_resolve_var(theme, ref);
        if(!value || s_parse_numbers(value, &n, 1) != 1) return fallback;
        return n;
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC s32 s_parse_line(UITheme *theme, char *line)
{
        char *eq;
        char *key;
        char *value;
        char *dot;
        char *colon;
        UIThemeClass *cls;
        UIThemeVar var;
        s32 state = -1;
        UIStyle *style;
        u32 *mask;
        const char *state_names[UI_STATE_COUNT];

        state_names[UI_STATE_HOVER] = "hover";
        state_names[UI_STATE_PRESSED] = "pressed";
        state_names[UI_STATE_FOCUSED] = "focused";
        state_names[UI_STATE_SELECTED] = "selected";
        state_names[UI_STATE_DISABLED] = "disabled";

        line = s_trim(line);
        if(*line == '\0' || *line == '#') return 0;

        eq = strchr(line, '=');
        if(!eq) return -1;
        *eq = '\0';
        key = s_trim(line);
        value = s_trim(eq + 1);

        /* Variable */
        if(key[0] == '$') {
                s32 i;

                s_copy(var.name, key + 1, UI_THEME_NAME_LEN);
                s_copy(var.value, value, UI_THEME_VALUE_LEN);

                /* Overwrite an existing variable */
                for(i = 0; i < list_count(&theme->vars); i++) {
                        UIThemeVar *v = list_get(&theme->vars, i);
                        if(strcmp(v->name, var.name) == 0) {
                                *v = var;
                                return 0;
                        }
                }
                return list_push(&theme->vars, 1, &var) < 0 ? -1 : 0;
        }

        /* class[:state].field, the field is after the last dot */
        dot = strrchr(key, '.');
        if(!dot || dot == key) return -1;
        *dot = '\0';

        colon = strchr(key, ':');
        if(colon) {
                *colon = '\0';
                state = s_parse_enum(colon + 1, state_names, UI_STATE_COUNT);
                if(state < 0) return -1;
        }

        cls = s_class_get_or_add(theme, key);
        if(!cls) return -1;

        if(state < 0) {
                style = &cls->base;
                mask = &cls->base_mask;
        } else {
                style = &cls->state[state];
                mask = &cls->state_mask[state];
        }
        return s_parse_field(theme, style, mask, dot + 1, value);
}

LOC UIThemeClass *s_class_get_or_add(UITheme *theme, const char *name)
{
        UIThemeClass cls;
        u64 idx;
        s32 i;

        if(hashmap_get(&theme->class_index, hash_str(name, HASH_SEED), &idx))
                return list_get(&theme->classes, (s32)idx - 1);

        memset(&cls, 0, sizeof(cls));
        s_copy(cls.name, name, UI_THEME_NAME_LEN);
        cls.base = ui_style_default();
        for(i = 0; i < UI_STATE_COUNT; i++)
                cls.state[i] = ui_style_default();

        if(list_push(&theme->classes, 1, &cls) < 0) return NULL;
        idx = (u64)list_count(&theme->classes);
        hashmap_set(&theme->class_index, hash_str(name, HASH_SEED), idx);
        return list_get(&theme->classes, (s32)idx - 1);
}

LOC const char *s_resolve_var(const UITheme *theme, const char *value)
{
        s32 depth;
        s32 i;
        s32 found;

        /* Follow $a = $b chains, but not forever */
        for(depth = 0; depth < 8 && value && value[0] == '$'; depth++) {
                found = 0;
                for(i = 0; i < list_count((List *)&theme->vars); i++) {
                        UIThemeVar *v = list_get((List *)&theme->vars, i);
                        if(strcmp(v->name, value + 1) == 0) {
                                value = v->value;
                                found = 1;
                                break;
                        }
                }
                if(!found) return NULL;
        }
        return value;
}

LOC s32 s_parse_field(const UITheme *theme, UIStyle *style, u32 *mask,
                const char *field, const char *value)
{
        const char *align[3];
        const char *layout[3];
        const char *overflow[3];
        const char *fonts[UI_FONT_COUNT];
        s32 e;

        align[0] = "start";
        align[1] = "center";
        align[2] = "end";
        layout[0] = "row";
        layout[1] = "column";
        layout[2] = "stack";
        overflow[0] = "visible";
        overflow[1] = "clip";
        overflow[2] = "scroll";
        fonts[UI_FONT_UI] = "ui";
        fonts[UI_FONT_UI_MEDIUM] = "ui-medium";
        fonts[UI_FONT_UI_BOLD] = "ui-bold";
        fonts[UI_FONT_CODE] = "code";

        value = s_resolve_var(theme, value);
        if(!value) return -1;

        if(strcmp(field, "width") == 0) {
                if(s_parse_size(value, &style->width) < 0) return -1;
                *mask |= UI_FIELD_WIDTH;
        } else if(strcmp(field, "height") == 0) {
                if(s_parse_size(value, &style->height) < 0) return -1;
                *mask |= UI_FIELD_HEIGHT;
        } else if(strcmp(field, "infill") == 0) {
                if(s_parse_color(theme, value, &style->infill) < 0) return -1;
                *mask |= UI_FIELD_INFILL;
        } else if(strcmp(field, "border-color") == 0) {
                if(s_parse_color(theme, value, &style->border_color) < 0)
                        return -1;
                *mask |= UI_FIELD_BORDER_COLOR;
        } else if(strcmp(field, "border-width") == 0) {
                if(s_parse_numbers(value, &style->border_width, 1) != 1)
                        return -1;
                *mask |= UI_FIELD_BORDER_WIDTH;
        } else if(strcmp(field, "corner-radius") == 0) {
                if(s_parse_corners(value, style->corner_radius) < 0) return -1;
                *mask |= UI_FIELD_CORNER_RADIUS;
        } else if(strcmp(field, "margin") == 0) {
                if(s_parse_sides(value, style->margin) < 0) return -1;
                *mask |= UI_FIELD_MARGIN;
        } else if(strcmp(field, "padding") == 0) {
                if(s_parse_sides(value, style->padding) < 0) return -1;
                *mask |= UI_FIELD_PADDING;
        } else if(strcmp(field, "gap") == 0) {
                if(s_parse_numbers(value, &style->gap, 1) != 1) return -1;
                *mask |= UI_FIELD_GAP;
        } else if(strcmp(field, "align-x") == 0) {
                if((e = s_parse_enum(value, align, 3)) < 0) return -1;
                style->align_x = (u8)e;
                *mask |= UI_FIELD_ALIGN_X;
        } else if(strcmp(field, "align-y") == 0) {
                if((e = s_parse_enum(value, align, 3)) < 0) return -1;
                style->align_y = (u8)e;
                *mask |= UI_FIELD_ALIGN_Y;
        } else if(strcmp(field, "layout") == 0) {
                if((e = s_parse_enum(value, layout, 3)) < 0) return -1;
                style->layout = (u8)e;
                *mask |= UI_FIELD_LAYOUT;
        } else if(strcmp(field, "wrap") == 0) {
                style->wrap = (strcmp(value, "wrap") == 0 ||
                                strcmp(value, "true") == 0 ||
                                strcmp(value, "1") == 0);
                *mask |= UI_FIELD_WRAP;
        } else if(strcmp(field, "overflow") == 0) {
                if((e = s_parse_enum(value, overflow, 3)) < 0) return -1;
                style->overflow = (u8)e;
                *mask |= UI_FIELD_OVERFLOW;
        } else if(strcmp(field, "font") == 0) {
                if((e = s_parse_enum(value, fonts, UI_FONT_COUNT)) < 0)
                        return -1;
                style->font = (u8)e;
                *mask |= UI_FIELD_FONT;
        } else if(strcmp(field, "font-size") == 0) {
                if(s_parse_numbers(value, &style->font_size, 1) != 1)
                        return -1;
                *mask |= UI_FIELD_FONT_SIZE;
        } else if(strcmp(field, "text-color") == 0) {
                if(s_parse_color(theme, value, &style->text_color) < 0)
                        return -1;
                *mask |= UI_FIELD_TEXT_COLOR;
        } else {
                return -1;
        }
        return 0;
}

LOC s32 s_parse_color(const UITheme *theme, const char *value, UIColor *out)
{
        u32 v;
        s32 len;
        char *end;

        value = s_resolve_var(theme, value);
        if(!value) return -1;

        if(strcmp(value, "none") == 0) {
                *out = ui_color_none();
                return 0;
        }
        if(value[0] != '#') return -1;

        len = (s32)strlen(value + 1);
        v = (u32)strtoul(value + 1, &end, 16);
        if(*end != '\0') return -1;

        if(len == 6) {
                *out = ui_hex((v << 8) | 0xff);
        } else if(len == 8) {
                *out = ui_hex(v);
        } else {
                return -1;
        }
        return 0;
}

LOC s32 s_parse_size(const char *value, UISize *out)
{
        f32 n;
        char *end;

        if(strcmp(value, "fit") == 0) {
                *out = ui_fit();
                return 0;
        }
        if(strncmp(value, "grow", 4) == 0) {
                n = 1.0f;
                if(value[4] != '\0' && s_parse_numbers(value + 4, &n, 1) != 1)
                        return -1;
                *out = ui_grow(n);
                return 0;
        }

        n = (f32)strtod(value, &end);
        if(end == value) return -1;
        while(isspace((u8)*end)) end++;

        if(*end == '%') {
                *out = ui_pct(n / 100.0f);
        } else if(*end == '\0' || strcmp(end, "px") == 0) {
                *out = ui_px(n);
        } else {
                return -1;
        }
        return 0;
}

LOC s32 s_parse_numbers(const char *value, f32 *out, s32 max)
{
        s32 count = 0;
        char *end;
        f32 n;

        while(count < max) {
                while(isspace((u8)*value)) value++;
                if(*value == '\0') break;

                n = (f32)strtod(value, &end);
                if(end == value) return -1;
                if(strncmp(end, "px", 2) == 0) end += 2;
                out[count++] = n;
                value = end;
        }
        while(isspace((u8)*value)) value++;
        return *value == '\0' ? count : -1;
}

/* CSS order: all | vertical horizontal | top right bottom left */
LOC s32 s_parse_sides(const char *value, f32 out[4])
{
        f32 n[4];
        s32 count = s_parse_numbers(value, n, 4);

        if(count == 1) {
                ui_sides(out, n[0]);
        } else if(count == 2) {
                ui_sides4(out, n[1], n[0], n[1], n[0]);
        } else if(count == 4) {
                ui_sides4(out, n[3], n[0], n[1], n[2]);
        } else {
                return -1;
        }
        return 0;
}

/* all | top-left top-right bottom-right bottom-left */
LOC s32 s_parse_corners(const char *value, f32 out[4])
{
        f32 n[4];
        s32 count = s_parse_numbers(value, n, 4);

        if(count == 1) {
                ui_radius(out, n[0]);
        } else if(count == 4) {
                ui_radius4(out, n[0], n[1], n[2], n[3]);
        } else {
                return -1;
        }
        return 0;
}

LOC s32 s_parse_enum(const char *value, const char **names, s32 count)
{
        s32 i;

        for(i = 0; i < count; i++) {
                if(strcmp(value, names[i]) == 0) return i;
        }
        return -1;
}

LOC char *s_trim(char *str)
{
        char *end;

        while(isspace((u8)*str)) str++;
        end = str + strlen(str);
        while(end > str && isspace((u8)end[-1])) end--;
        *end = '\0';
        return str;
}

LOC void s_copy(char *dst, const char *src, s32 size)
{
        s32 len = (s32)strlen(src);

        if(len >= size) len = size - 1;
        memcpy(dst, src, (size_t)len);
        dst[len] = '\0';
}
