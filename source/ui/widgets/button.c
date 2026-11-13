#include <ui/widgets/button.h>

#include <ui/widgets/tooltip.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UISignal ui_button(UI *ui, const char *label, const char *cls)
{
        UIBox *box = ui_box(ui, label, cls ? cls : "button", UI_BOX_CLICKABLE);

        box->cursor = UI_CURSOR_POINTER;
        ui_box_set_text(ui, box, label);
        return ui_signal(ui, box);
}

PUB UISignal ui_icon_button(UI *ui, const char *id, u16 icon,
                const char *tooltip, const char *cls)
{
        UIBox *box = ui_box(ui, id, cls ? cls : "icon-button",
                        UI_BOX_CLICKABLE);

        box->icon = icon;
        box->cursor = UI_CURSOR_POINTER;
        if(tooltip) ui_tooltip(ui, box, tooltip);
        return ui_signal(ui, box);
}
