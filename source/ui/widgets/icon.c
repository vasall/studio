#include <ui/widgets/icon.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UIBox *ui_icon(UI *ui, u16 icon, const char *cls)
{
        UIBox *box = ui_box(ui, NULL, cls ? cls : "icon", 0);

        box->icon = icon;
        return box;
}
