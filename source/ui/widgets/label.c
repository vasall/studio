#include <ui/widgets/label.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UIBox *ui_label(UI *ui, const char *text, const char *cls)
{
        UIBox *box = ui_box(ui, NULL, cls ? cls : "label", 0);

        ui_box_set_text(ui, box, text);
        return box;
}
