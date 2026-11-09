#include <graphic/mesh/face.h>

PUB Face face(s32 i0, s32 i1, s32 i2)
{
        Face fc;
        fc.indices[0] = i0;
        fc.indices[1] = i1;
        fc.indices[2] = i2;
        return fc;
}
