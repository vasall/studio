#include <shape/polygon.h>

#include <devkit/log/log.h>

PUB s32 poly_init(Polygon *poly)
{
        if(list_init(&poly->points, sizeof(Point), 32) < 0) {
                LOG(LOG_ERROR, "Failed to create list for polygon");
                return -1;
        }

        return 0;
}

PUB void poly_deinit(Polygon *poly)
{
        list_deinit(&poly->points);
}

PUB s32 poly_count(Polygon *poly)
{
        return list_count(&poly->points);
}

PUB void poly_clear(Polygon *poly)
{
        list_clear(&poly->points);
}

PUB s32 poly_push(Polygon *poly, Point pnt)
{
        return list_push(&poly->points, 1, &pnt);
}

PUB s32 poly_push_num(Polygon *poly, s32 num, Point *pnt)
{
        return list_push(&poly->points, num, pnt);
}