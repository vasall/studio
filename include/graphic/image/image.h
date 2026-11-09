#ifndef VASALL_IMAGE_H
#define VASALL_IMAGE_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <shape/point.h>
#include <shape/rectangle.h>
#include <shape/circle.h>
#include <shape/polygon.h>

#include <graphic/general/color.h>

typedef struct Image {
        s32 width;
        s32 height;
        s64 pixel_count;
        IColor *pixels;
} Image;

/*
 * Create a new image.
 *
 * @param img Pointer to the image
 * @param w The width of the image
 * @param h The height of the image
 * 
 * @returns 0 on success or -1 if an error occurred
*/
PUB s32 img_init(Image *img, s32 w, s32 h);

/*
 *
*/
PUB void img_deinit(Image *img);

/*
 *
*/
PUB s32 img_resize(Image *img, s32 w, s32 h);

/*
 * Extract a part of the image and copy it to a new image.
*/
PUB s32 img_crop(Image *img_in, Rectangle rect, Image *img_out);

/*
 * Fill the entire image with the given color.
*/
PUB void img_fill(Image *img, IColor color);

/*
 * Set the pixels of the image in the given rect to the color.
*/
PUB void img_set_rect(Image *img, Rectangle rect, IColor col);

/*
 * Draw a line from (x1, y1) to (x2, y2) with the given color and line width.
*/
PUB void img_draw_line(Image *img, s32 x1, s32 y1, s32 x2, s32 y2,
        IColor col, s32 w);

/*
 *
*/
PUB void img_fill_rect(Image *img, Rectangle rect, IColor col);

/*
 * Draw the outline of a rectangle with the given color and line width.
 * The outline will end half out and half in.
*/
PUB void img_draw_rect(Image *img, Rectangle rect, IColor col, s32 w);

/*
 *
*/
PUB void img_fill_circle(Image *img, Circle circ, IColor col);

/*
 *
*/
PUB void img_draw_circle(Image *img, Circle circ, IColor col, s32 w);

/*
 * Fill a part of a circle (a pie slice) from the start to the end angle.
 *
 * Angles are in radians. 0 points right and angles grow clockwise on screen,
 * so pi / 2 points down. The arc always goes clockwise from start to end, so
 * an end smaller than the start wraps around. A sweep of 2 pi or more fills
 * the whole circle, equal angles draw nothing.
 *
 * The start edge is included and the end edge is not, so slices that meet
 * (e.g. in a pie chart) don't overlap.
 *
 * @param img Pointer to the image
 * @param circ The circle
 * @param start The start angle in radians
 * @param end The end angle in radians
 * @param col The color
*/
PUB void img_fill_arc(Image *img, Circle circ, f32 start, f32 end,
                IColor col);

/*
 * Draw a part of a circle's outline from the start to the end angle, with
 * the given line width. The outline is centered on the radius and the ends
 * are cut straight along the radius. Angles work like in img_fill_arc.
 *
 * @param img Pointer to the image
 * @param circ The circle
 * @param start The start angle in radians
 * @param end The end angle in radians
 * @param col The color
 * @param w The line width
*/
PUB void img_draw_arc(Image *img, Circle circ, f32 start, f32 end,
                IColor col, s32 w);

/*
 * Fill a polygon. The points are connected in order and the last point is
 * connected back to the first.
 *
 * The points lie on pixel corners, so the square (0, 0), (10, 0), (10, 10),
 * (0, 10) fills the same 10x10 pixels as img_fill_rect. Self overlapping
 * polygons (like a star drawn with 5 points) are filled completely
 * (nonzero rule). Polygons that share an edge don't overlap.
 *
 * @param img Pointer to the image
 * @param poly The polygon, with at least 3 points
 * @param col The color
*/
PUB void img_fill_polygon(Image *img, Polygon poly, IColor col);

/*
 * Draw the outline of a polygon with the given line width. The last point
 * is connected back to the first. Like img_draw_line, the outline is
 * centered on the edges and the corners are round. Every pixel is blended
 * only once, also where edges meet.
 *
 * @param img Pointer to the image
 * @param poly The polygon
 * @param col The color
 * @param w The line width
*/
PUB void img_draw_polygon(Image *img, Polygon poly, IColor col, s32 w);


#endif /* VASALL_IMAGE_H */