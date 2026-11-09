#include <graphic/mesh/vertex.h>

PUB Vertex vertex(FVector3 pos, FVector3 norm, FVector2 uv, IColor col)
{
        Vertex vtx;
        vtx.position = pos;
        vtx.normal = norm;
        vtx.uv = uv;
        vtx.color = col;
        return vtx;
}
