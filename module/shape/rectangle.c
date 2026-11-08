#include <shape/rectangle.h>

#include <devkit/util/helper.h>

PUB Rectangle mkrect(s32 x, s32 y, s32 w, s32 h)
{
        Rectangle rect;
        rect.x = x;
        rect.y = y;
        rect.w = w;
        rect.h = h;
        return rect;
}

PUB s32 rect_contains(Rectangle rect, s32 x, s32 y)
{
        if(x < rect.x || x > rect.x + rect.w) return -1;
        if(y < rect.y || y > rect.y + rect.h) return -1;
        return 0;
}

PUB s32 rect_intersect(Rectangle a, Rectangle b, Rectangle *rect_out)
{
        /* The overlap spans from the larger start to the smaller end */
        s32 minx = MAX(a.x, b.x);
        s32 miny = MAX(a.y, b.y);
        s32 maxx = MIN(a.x + a.w, b.x + b.w);
        s32 maxy = MIN(a.y + a.h, b.y + b.h);

        /* Rectangles that only touch along an edge do not intersect */
        if(maxx <= minx || maxy <= miny) {
                return -1;
        }

        if(rect_out) {
                *rect_out = mkrect(minx, miny, maxx - minx, maxy - miny);
        }

        return 0;
}

PUB Rectangle rect_union(Rectangle a, Rectangle b)
{
        Rectangle rect;
        s32 minx = MIN(a.x, b.x);
        s32 miny = MIN(a.y, b.y);
        s32 maxx = MAX(a.x + a.w, b.x + b.w);
        s32 maxy = MAX(a.y + a.h, b.y + b.h);
        rect.x = minx;
        rect.y = miny;
        rect.w = maxx - minx;
        rect.h = maxy - miny;
        return rect;
}