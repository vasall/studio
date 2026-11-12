#include <ui/debug.h>
#include <ui/widgets.h>

#include <stdio.h>
#include <string.h>

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

LOC void s_outline(UI *ui, UIBox *box, s32 depth);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB void ui_debug_panel(UI *ui, const char *info)
{
        UIBox *panel;
        UIBoxState *st;
        char buf[160];

        if(!ui->debug) return;

        ui_layer_begin(ui, UI_LAYER_DEBUG);
        panel = ui_box_begin(ui, "debug-panel", "debug-panel",
                        UI_BOX_FLOATING | UI_BOX_PASSIVE);
        panel->style.layout = UI_LAYOUT_COLUMN;

        /* Bottom right, using last frame's size */
        st = panel->state;
        panel->float_x = ui->width - (st ? st->rect.w : 0.0f) - 8.0f;
        panel->float_y = ui->height - (st ? st->rect.h : 0.0f) - 30.0f;

        snprintf(buf, sizeof(buf), "boxes %d  states %d  cmds %d",
                        ui->box_count, pool_count(&ui->states),
                        list_count(&ui->cmds));
        ui_label(ui, buf, "debug-text");

        snprintf(buf, sizeof(buf), "hot %08x  active %08x  focus %08x",
                        (u32)ui->hot, (u32)ui->active, (u32)ui->focus);
        ui_label(ui, buf, "debug-text");

        snprintf(buf, sizeof(buf), "mouse %.0f, %.0f  layer %d",
                        ui->input ? ui->input->mouse_x : 0.0f,
                        ui->input ? ui->input->mouse_y : 0.0f,
                        ui->hot_layer);
        ui_label(ui, buf, "debug-text");

        if(info) ui_label(ui, info, "debug-text");

        ui_box_end(ui); /* debug-panel */
        ui_layer_end(ui);
}

PUB void ui_debug_outlines(UI *ui)
{
        s32 i;

        for(i = 0; i < UI_LAYER_DEBUG; i++)
                s_outline(ui, ui->roots[i], 0);
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/

LOC void s_outline(UI *ui, UIBox *box, s32 depth)
{
        static const u32 colors[4] = {
                0xff5f5fa0, 0x5fff5fa0, 0x5f9fffa0, 0xffdf5fa0
        };
        UIDrawCmd cmd;
        UIBox *c;

        if(box->rect.w > 0.0f && box->rect.h > 0.0f) {
                memset(&cmd, 0, sizeof(cmd));
                cmd.kind = UI_DRAW_RECT;
                cmd.rect = box->rect;
                cmd.clip = ui_rect(0.0f, 0.0f, ui->width, ui->height);
                cmd.border_width = 1.0f;
                cmd.border_color = ui_hex(colors[depth % 4]);
                if(box->key && box->key == ui->hot) {
                        cmd.color = ui_hex(0xffffff20);
                        cmd.border_color = ui_hex(0xffffffff);
                }
                ui_draw_push(ui, &cmd);
        }

        for(c = box->first; c; c = c->next)
                s_outline(ui, c, depth + 1);
}
