#ifndef VASALL_POINT_H
#define VASALL_POINT_H

#include <devkit/scope.h>
#include <devkit/types.h>

typedef struct Point {
        s32 x;
        s32 y;
} Point;

PUB Point mkpoint(s32 x, s32 y);


#endif /* VASALL_POINT_H */