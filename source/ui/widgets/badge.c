#include <ui/widgets/badge.h>

#include <ui/widgets/label.h>

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB UIBox *ui_badge(UI *ui, const char *text, const char *cls)
{
        return ui_label(ui, text, cls ? cls : "badge");
}
