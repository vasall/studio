#include <ui/widgets/spacer.h>

#include <stddef.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UIBox *ui_spacer(UI *ui, UISize w, UISize h)
{
        UIBox *box = ui_box(ui, NULL, NULL, 0);

        box->style.width = w;
        box->style.height = h;
        return box;
}
