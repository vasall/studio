#ifndef VASALL_STUDIO_UI_WIDGETS_TEXT_FIELD_H
#define VASALL_STUDIO_UI_WIDGETS_TEXT_FIELD_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <ui/ui.h>

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

typedef struct UITextFieldResult {
        UISignal sig;
        UIBoxKey key;           /* Key of the field, e.g. for ui_set_focus */
        s32 changed;            /* The text was edited this frame */
        s32 submitted;          /* Enter was pressed */
        s32 cancelled;          /* Esc was pressed; focus is dropped */
        s32 focused;
} UITextFieldResult;

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Single-line text field that edits buf (UTF-8, cap bytes incl. the
 * terminator) in place. Mouse: click, drag and double-click to select.
 * Keys: arrows, Home/End, Backspace/Delete (with Ctrl by word), Shift to
 * select, Ctrl+A/C/X/V, Enter submits, Esc cancels. placeholder is shown
 * while buf is empty. cls defaults to "text-field"; the selection uses
 * "text-field-selection" and the placeholder "text-field-placeholder".
 */
PUB UITextFieldResult ui_text_field(UI *ui, const char *id, char *buf,
                s32 cap, const char *placeholder, const char *cls);

#endif /* VASALL_STUDIO_UI_WIDGETS_TEXT_FIELD_H */
