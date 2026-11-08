#ifndef VASALL_CIRCLE_H
#define VASALL_CIRCLE_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				    ICIRCLE
 *
 ******************************************************************************/

typedef struct Circle {
        s32 x;
        s32 y;
        s32 r;
} Circle;

PUB Circle mkcircle(s32 x, s32 y, s32 r);

#endif /* VASALL_CIRCLE_H */