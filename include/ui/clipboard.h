#ifndef VASALL_STUDIO_CLIPBOARD_H
#define VASALL_STUDIO_CLIPBOARD_H

#include <devkit/scope.h>
#include <devkit/types.h>


/**
 * Clipboard of the platform. get copies at most cap - 1 bytes into buf and
 * terminates it; it returns the length.
 */
typedef s32 (*UIClipboardGetFn)(void *user, char *buf, s32 cap);
typedef void (*UIClipboardSetFn)(void *user, const char *text, s32 len);

#endif /* VASALL_STUDIO_CLIPBOARD_H */
