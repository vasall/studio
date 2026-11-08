#include <shape/circle.h>

PUB Circle mkcircle(s32 x, s32 y, s32 r)
{
        Circle circ;
        circ.x = x;
        circ.y = y;
        circ.r = r;
        return circ;
}