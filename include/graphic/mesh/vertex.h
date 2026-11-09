#ifndef VASALL_VERTEX_H
#define VASALL_VERTEX_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/math/vector.h>
#include <graphic/general/color.h>

typedef struct Vertex {
        FVector3 position;
        FVector3 normal;
        FVector2 uv;
        IColor color;
} Vertex;

PUB Vertex vertex(FVector3 pos, FVector3 norm, FVector2 uv, IColor col);

#endif /* VASALL_VERTEX_H */
