#ifndef VASALL_POLYGON_H
#define VASALL_POLYGON_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/memory/list.h>

#include <shape/point.h>


typedef struct Polygon {
        List points;
} Polygon;

/*
 *
*/
PUB s32 poly_init(Polygon *poly);

/*
 *
*/
PUB void poly_deinit(Polygon *poly);

/*
 *
*/
PUB s32 poly_count(Polygon *poly);

/*
 *
*/
PUB void poly_clear(Polygon *poly);

/*
 *
*/
PUB s32 poly_push(Polygon *poly, Point pnt);

/*
 *
*/
PUB s32 poly_push_num(Polygon *poly, s32 num, Point *pnt);

#endif /* VASALL_POLYGON_H */