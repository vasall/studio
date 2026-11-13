#ifndef VASALL_STUDIO_UI_WIDGETS_TAB_H
#define VASALL_STUDIO_UI_WIDGETS_TAB_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct UITabResult {
        UISignal sig;           /* Of the tab itself */
        UIBoxKey key;           /* Key of the tab box (for its state/rect) */
        s32 close_clicked;
} UITabResult;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * An editor tab with title, optional icon and close button. The close
 * button shows on the selected tab and on hover. The selected tab gets an
 * accent line on top. cls defaults to "tab".
 */
PUB UITabResult ui_tab(UI *ui, const char *id, const char *title, u16 icon,
                s32 selected, s32 closable, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_TAB_H */
