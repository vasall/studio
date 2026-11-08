#ifndef VASALL_RECTANGLE_H
#define VASALL_RECTANGLE_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				    rectANGLE
 *
 ******************************************************************************/

typedef struct Rectangle {
        s32 x;
        s32 y;
        s32 w;
        s32 h;
} Rectangle;

/*
 *
*/
PUB Rectangle mkrect(s32 x, s32 y, s32 w, s32 h);

/*
 *
*/
PUB s32 rect_contains(Rectangle rect, s32 x, s32 y);

/*
 * Check if rectangles A and B intersect. Rectangles that only touch along
 * an edge do not intersect.
 *
 * @param a The first rectangle
 * @param b The second rectangle
 * @param rect_out Receives the overlapping area, or an empty rectangle if
 *                 there is none (may be NULL)
 *
 * @returns 0 if the rectangles intersect, -1 if not
*/
PUB s32 rect_intersect(Rectangle a, Rectangle b, Rectangle *rect_out);

/*
 * Create a new rectangle containing both rectangle A and B.
*/
PUB Rectangle rect_union(Rectangle a, Rectangle b);

#endif /* VASALL_RECTANGLE_H */