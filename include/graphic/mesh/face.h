#ifndef VASALL_FACE_H
#define VASALL_FACE_H

#include <devkit/scope.h>
#include <devkit/types.h>

typedef struct Face {
        s32 indices[3];
} Face;

PUB Face face(s32 i0, s32 i1, s32 i2);

#endif /* VASALL_FACE_H */
