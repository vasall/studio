#include <graphic/image/image.h>

#include <devkit/log/log.h>
#include <devkit/util/helper.h>

#include <stdlib.h>
#include <math.h>

/******************************************************************************
 *
 * 				 DEFINES/MACROS
 *
 ******************************************************************************/

#define S_PI     3.14159265358979323846f
#define S_TWO_PI 6.28318530717958647692f

/* Tolerance for angles that are equal except for float error */
#define S_ANGLE_EPS 1e-5f

/*
 * Tolerance for direction components that are 0 except for float error.
 * Without it pixels lying exactly on a horizontal or vertical edge could end
 * up on the wrong side of it.
 */
#define S_DIR_EPS 1e-6f

/******************************************************************************
 *
 * 				   STRUCTURES
 *
 ******************************************************************************/

/*
 * The angle range of an arc, stored as the start and end direction. Pixels
 * are tested with cross products, so no trigonometry is needed per pixel.
 */
typedef struct Sector {
        f32 sx;         /* Start direction */
        f32 sy;
        f32 ex;         /* End direction */
        f32 ey;
        s32 wide;       /* 1 if the sweep is larger than half a circle */
} Sector;

/* Where a polygon edge crosses a row, for the scanline fill */
typedef struct Crossing {
        f32 x;          /* Position of the crossing */
        s32 dir;        /* +1 if the edge goes down, -1 if it goes up */
} Crossing;

/* A range of pixels x0 .. x1 (inclusive) in one row */
typedef struct Span {
        s32 x0;
        s32 x1;
} Span;

/******************************************************************************
 *
 * 				LOCAL INTERFACE
 *
 ******************************************************************************/

/*
 * Set the color of the pixel at (x, y). This will not blend, but overwrite the
 * pixel.
*/
LOC void s_set_pixel_xy(Image *img, s32 x, s32 y, IColor col);

/*
 * Set the color of the i-th pixel. This will not blend, but overwrite the
 * pixel.
*/
LOC void s_set_pixel(Image *img, s32 i, IColor col);

/*
 * Blend the color over the pixel at (x, y). The pixels of the image are
 * stored with premultiplied alpha, the color is passed with straight alpha.
 */
LOC void s_blend_pixel_xy(Image *img, s32 x, s32 y, IColor col);

/*
 * Blend the color over the i-th pixel. The pixels of the image are stored
 * with premultiplied alpha, the color is passed with straight alpha.
*/
LOC void s_blend_pixel(Image *img, s32 i, IColor col);

/*
 * Blend the color over the pixels x0 .. x1 (inclusive) of row y. The span is
 * clipped to the image.
 */
LOC void s_blend_span(Image *img, s32 y, s32 x0, s32 x1, IColor col);

/*
 * Set up the sector from start to end, going clockwise on screen.
 *
 * @returns 1 if the sector covers the full circle, 0 if it is a part of the
 *          circle, -1 if it is empty
 */
LOC s32 s_sector_init(Sector *sec, f32 start, f32 end);

/*
 * Check if the offset (dx, dy) from the center lies inside the sector. The
 * start edge is included and the end edge is not, so two arcs that meet
 * don't share pixels. The center pixel is always inside.
 */
LOC s32 s_sector_contains(const Sector *sec, s32 dx, s32 dy);

/*
 * Blend the pixels x0 .. x1 of row y, but only those inside the sector. A
 * NULL sector means the full circle.
 */
LOC void s_arc_span(Image *img, Circle circ, const Sector *sec, s32 y,
                s32 x0, s32 x1, IColor col);

/*
 * Blend the ring part of one row: the pixels with hole < |dx| <= outer. A
 * negative hole means the row doesn't cross the hole.
 */
LOC void s_ring_row(Image *img, Circle circ, const Sector *sec, s32 y,
                s32 outer, s32 hole, IColor col);

/*
 * Fill the part of the circle inside the sector (NULL for the full circle).
 */
LOC void s_fill_circle(Image *img, Circle circ, const Sector *sec,
                IColor col);

/*
 * Draw the outline of the circle inside the sector (NULL for the full circle).
 */
LOC void s_draw_circle(Image *img, Circle circ, const Sector *sec,
                IColor col, s32 w);

/*
 * Copy the points of the polygon into a new array and allocate the scratch
 * buffer used per row (room for one Crossing or Span per edge). Both live in
 * one allocation that is freed with free(*pts).
 *
 * @returns The number of points, or -1 if an error occurred
 */
LOC s32 s_poly_prepare(Polygon *poly, Point **pts, void **scratch);

/*
 * Check if the center of pixel (x, y) is within w / 2 of the segment from a
 * to b, exactly and without float error. Points lie on pixel corners, as in
 * img_draw_line.
 *
 * @returns 1 if the pixel is inside, 0 if not
 */
LOC s32 s_in_capsule(Point a, Point b, s32 w, s32 x, s32 y);

/*
 * Get the range of x positions (as pixel centers) where a row at height sy
 * is within r of the segment from a to b.
 *
 * @returns 1 if the row touches the segment, 0 if not
 */
LOC s32 s_capsule_row(Point a, Point b, f32 r, f32 sy, f32 *left,
                f32 *right);

/******************************************************************************
 *
 * 				PUBLIC FUNCTIONS
 *
 ******************************************************************************/

PUB s32 img_init(Image *img, s32 w, s32 h)
{
        s64 size;

        size = w * h * sizeof(IColor);
        if(!(img->pixels = malloc(size))) {
                LOG(LOG_ERROR, "Failed to allocate memory for pixels");
                return -1;
        }

        img->width = w;
        img->height = h;
        img->pixel_count = w * h;

        return 0;
}

PUB void img_deinit(Image *img)
{
        free(img->pixels);
}

PUB s32 img_resize(Image *img, s32 w, s32 h)
{
        IColor *ptr;
        s64     size;

        size = w * h * sizeof(IColor);
        if(!(ptr = realloc(img->pixels, size))) {
                LOG(LOG_ERROR, "Failed to reallocate pixel buffer");
                return -1;
        }

        img->pixels = ptr;
        img->width = w;
        img->height = h;
        img->pixel_count = w * h;

        return 0;
}

PUB void img_fill(Image *img, IColor col)
{
        s64 i;

        /* Overwrite instead of blend, but store premultiplied like all pixels */
        col = icolor_premultiply(col);

        for(i = 0; i < img->width * img->height; i++) {
                s_set_pixel(img, i, col);
        }
}

PUB void img_set_rect(Image *img, Rectangle rect, IColor col)
{
        s32 x;
        s32 y;
        s32 row;

        /* Overwrite instead of blend, but store premultiplied like all pixels */
        col = icolor_premultiply(col);

        for(y = rect.y; y < rect.y + rect.h; y++) {
                row = y * img->width;
                for(x = rect.x; x < rect.x + rect.w; x++) {
                        s_set_pixel(img, row + x, col);
                }
        }
}

PUB void img_draw_line(Image *img, s32 x1, s32 y1, s32 x2, s32 y2, IColor col,
                       s32 w)
{
        f32 half = w / 2.0f;
        Point a = mkpoint(x1, y1);
        Point b = mkpoint(x2, y2);
        s32 minx;
        s32 miny;
        s32 maxx;
        s32 maxy;
        s32 x;
        s32 y;

        /* 
         * Only check pixels inside the line's bounding box, grown by half
         * the width and clipped to the image.
         */
        minx = (s32)floorf(MIN(x1, x2) - half);
        miny = (s32)floorf(MIN(y1, y2) - half);
        maxx = (s32)ceilf(MAX(x1, x2) + half);
        maxy = (s32)ceilf(MAX(y1, y2) + half);
        if(minx < 0) minx = 0;
        if(miny < 0) miny = 0;
        if(maxx > img->width - 1) maxx = img->width - 1;
        if(maxy > img->height - 1) maxy = img->height - 1;

        /*
         * The loop range is clipped to the image above, so the pixels
         * don't need a bounds check.
         */
        for(y = miny; y <= maxy; y++) {
                s32 row = y * img->width;

                for(x = minx; x <= maxx; x++) {
                        if(s_in_capsule(a, b, w, x, y)) {
                                s_blend_pixel(img, row + x, col);
                        }
                }
        }
}

PUB void img_fill_rect(Image *img, Rectangle rect, IColor col)
{
        /* Clip to the image; the rectangle covers x .. x + w - 1 */
        s32 x0 = MAX(rect.x, 0);
        s32 y0 = MAX(rect.y, 0);
        s32 x1 = MIN(rect.x + rect.w, img->width);
        s32 y1 = MIN(rect.y + rect.h, img->height);
        s32 x;
        s32 y;

        for(y = y0; y < y1; y++) {
                s32 row = y * img->width;
                for(x = x0; x < x1; x++) {
                        s_blend_pixel(img, row + x, col);
                }
        }
}

PUB void img_draw_rect(Image *img, Rectangle rect, IColor col, s32 w)
{
        s32 hw = w / 2;
        Rectangle o;
        s32 side_h;

        if(w <= 0) {
                return;
        }

        /* Outer box: the rectangle grown by half the line width on each side */
        o = mkrect(rect.x - hw, rect.y - hw, rect.w + 2 * hw, rect.h + 2 * hw);

        /* Border thicker than the box: the whole box is border */
        side_h = o.h - 2 * w;
        if(side_h <= 0 || o.w <= 2 * w) {
                img_fill_rect(img, o, col);
                return;
        }

        /* Top and bottom span the full width including the corners */
        img_fill_rect(img, mkrect(o.x, o.y, o.w, w), col);
        img_fill_rect(img, mkrect(o.x, o.y + o.h - w, o.w, w), col);

        /* Left and right only cover the height between them */
        img_fill_rect(img, mkrect(o.x, o.y + w, w, side_h), col);
        img_fill_rect(img, mkrect(o.x + o.w - w, o.y + w, w, side_h), col);
}

PUB void img_fill_circle(Image *img, Circle circ, IColor col)
{
        s_fill_circle(img, circ, NULL, col);
}

PUB void img_draw_circle(Image *img, Circle circ, IColor col, s32 w)
{
        s_draw_circle(img, circ, NULL, col, w);
}

PUB void img_fill_arc(Image *img, Circle circ, f32 start, f32 end,
                IColor col)
{
        Sector sec;
        s32 kind = s_sector_init(&sec, start, end);

        if(kind < 0) {
                return;
        }

        s_fill_circle(img, circ, kind == 1 ? NULL : &sec, col);
}

PUB void img_draw_arc(Image *img, Circle circ, f32 start, f32 end,
                IColor col, s32 w)
{
        Sector sec;
        s32 kind = s_sector_init(&sec, start, end);

        if(kind < 0) {
                return;
        }

        s_draw_circle(img, circ, kind == 1 ? NULL : &sec, col, w);
}

PUB void img_fill_polygon(Image *img, Polygon poly, IColor col)
{
        Point *pts;
        void *scratch;
        Crossing *cross;
        s32 n;
        s32 miny;
        s32 maxy;
        s32 y;
        s32 i;

        if((n = s_poly_prepare(&poly, &pts, &scratch)) < 3) {
                if(n > 0) {
                        free(pts);
                }
                return;
        }
        cross = scratch;

        /* Only visit the rows the polygon covers that are inside the image */
        miny = maxy = pts[0].y;
        for(i = 1; i < n; i++) {
                miny = MIN(miny, pts[i].y);
                maxy = MAX(maxy, pts[i].y);
        }
        miny = MAX(miny, 0);
        maxy = MIN(maxy, img->height);

        /*
         * The vertices lie on pixel corners and each row is sampled at its
         * pixel centers (y + 0.5). Because vertices are whole numbers, a row
         * never goes exactly through a vertex, so no crossing is counted
         * twice.
         */
        for(y = miny; y < maxy; y++) {
                f32 sy = y + 0.5f;
                s32 num = 0;
                s32 winding = 0;

                /* Collect where the edges cross this row */
                for(i = 0; i < n; i++) {
                        Point a = pts[i];
                        Point b = pts[(i + 1) % n];
                        Crossing c;
                        s32 k;

                        if((sy < a.y) == (sy < b.y)) {
                                continue;
                        }

                        c.x = a.x + (sy - a.y) * (b.x - a.x) / (f32)(b.y - a.y);
                        c.dir = b.y > a.y ? 1 : -1;

                        /* Insertion sort, rows only have a few crossings */
                        for(k = num; k > 0 && cross[k - 1].x > c.x; k--) {
                                cross[k] = cross[k - 1];
                        }
                        cross[k] = c;
                        num++;
                }

                /*
                 * Nonzero rule: fill where the edges crossed so far don't
                 * cancel out. That fills self overlapping shapes like a star
                 * completely. A pixel is filled if its center lies in
                 * [left, right), so polygons sharing an edge don't overlap.
                 */
                for(i = 0; i < num - 1; i++) {
                        winding += cross[i].dir;
                        if(winding != 0) {
                                s32 x0 = (s32)ceilf(cross[i].x - 0.5f);
                                s32 x1 = (s32)ceilf(cross[i + 1].x - 0.5f) - 1;

                                if(x0 <= x1) {
                                        s_blend_span(img, y, x0, x1, col);
                                }
                        }
                }
        }

        free(pts);
}

PUB void img_draw_polygon(Image *img, Polygon poly, IColor col, s32 w)
{
        Point *pts;
        void *scratch;
        Span *spans;
        f32 r = w / 2.0f;
        s32 reach = (s32)ceilf(r);
        s32 n;
        s32 miny;
        s32 maxy;
        s32 y;
        s32 i;

        if(w <= 0) {
                return;
        }

        if((n = s_poly_prepare(&poly, &pts, &scratch)) < 1) {
                return;
        }
        spans = scratch;

        /* Rows the outline covers, grown by half the line width and clipped */
        miny = maxy = pts[0].y;
        for(i = 1; i < n; i++) {
                miny = MIN(miny, pts[i].y);
                maxy = MAX(maxy, pts[i].y);
        }
        miny = MAX(miny - reach, 0);
        maxy = MIN(maxy + reach, img->height - 1);

        /*
         * Every edge is a thick line with round ends, like img_draw_line, so
         * corners get round joins. Instead of drawing the edges one after
         * another (which would blend corners twice), each row collects the
         * pixel range of every edge, merges overlapping ranges and fills
         * every pixel once.
         */
        for(y = miny; y <= maxy; y++) {
                f32 sy = y + 0.5f;
                s32 num = 0;

                for(i = 0; i < n; i++) {
                        Point a = pts[i];
                        Point b = pts[(i + 1) % n];
                        f32 left;
                        f32 right;
                        Span sp;
                        s32 k;

                        if(!s_capsule_row(a, b, r, sy, &left, &right)) {
                                continue;
                        }

                        /*
                         * Pixels whose center x + 0.5 lies in [left, right].
                         * The float range can be off by a pixel at the
                         * exact edge, so check both ends with the exact
                         * test and move them if needed.
                         */
                        sp.x0 = (s32)ceilf(left - 0.5f);
                        sp.x1 = (s32)floorf(right - 0.5f);
                        while(sp.x0 <= sp.x1 &&
                                        !s_in_capsule(a, b, w, sp.x0, y)) {
                                sp.x0++;
                        }
                        while(sp.x1 >= sp.x0 &&
                                        !s_in_capsule(a, b, w, sp.x1, y)) {
                                sp.x1--;
                        }
                        if(sp.x0 > sp.x1) {
                                continue;
                        }
                        while(s_in_capsule(a, b, w, sp.x0 - 1, y)) {
                                sp.x0--;
                        }
                        while(s_in_capsule(a, b, w, sp.x1 + 1, y)) {
                                sp.x1++;
                        }

                        /* Insertion sort by start */
                        for(k = num; k > 0 && spans[k - 1].x0 > sp.x0; k--) {
                                spans[k] = spans[k - 1];
                        }
                        spans[k] = sp;
                        num++;
                }

                /* Merge overlapping or touching ranges and fill each once */
                for(i = 0; i < num; ) {
                        s32 x0 = spans[i].x0;
                        s32 x1 = spans[i].x1;

                        for(i++; i < num && spans[i].x0 <= x1 + 1; i++) {
                                x1 = MAX(x1, spans[i].x1);
                        }

                        s_blend_span(img, y, x0, x1, col);
                }
        }

        free(pts);
}

PUB void img_blend_mask(Image *img, s32 x, s32 y, const u8 *mask, s32 w,
                s32 h, IColor col)
{
        s32 x0;
        s32 y0;
        s32 x1;
        s32 y1;
        s32 mx;
        s32 my;

        if(!img || !mask || w < 1 || h < 1) {
                return;
        }

        /* Clip to the image; the mask covers x .. x + w - 1 */
        x0 = MAX(x, 0);
        y0 = MAX(y, 0);
        x1 = MIN(x + w, img->width);
        y1 = MIN(y + h, img->height);

        for(my = y0; my < y1; my++) {
                const u8 *row = mask + (my - y) * w;
                s32 out = my * img->width;

                for(mx = x0; mx < x1; mx++) {
                        u8 cover = row[mx - x];
                        IColor c = col;

                        if(cover == 0) {
                                continue;
                        }

                        /* Rounded col.a * cover / 255 */
                        c.a = (u8)((col.a * cover + 127) / 255);
                        s_blend_pixel(img, out + mx, c);
                }
        }
}

/******************************************************************************
 *
 * 				LOCAL FUNCTIONS
 *
 ******************************************************************************/


LOC void s_set_pixel_xy(Image *img, s32 x, s32 y, IColor col)
{
        s32 index = y * img->width + x;
        img->pixels[index] = col;
}

LOC void s_set_pixel(Image *img, s32 i, IColor col)
{
        img->pixels[i] = col;
}

LOC void s_blend_pixel_xy(Image *img, s32 x, s32 y, IColor col)
{
        IColor *pixel;

        pixel = &img->pixels[y * img->width + x];
        *pixel = icolor_blend(*pixel, col);
}

LOC void s_blend_pixel(Image *img, s32 i, IColor col)
{
        IColor *pixel;

        pixel = &img->pixels[i];
        *pixel = icolor_blend(*pixel, col);
}

LOC void s_blend_span(Image *img, s32 y, s32 x0, s32 x1, IColor col)
{
        s32 row;
        s32 x;

        if(y < 0 || y >= img->height) {
                return;
        }

        x0 = MAX(x0, 0);
        x1 = MIN(x1, img->width - 1);
        row = y * img->width;

        for(x = x0; x <= x1; x++) {
                s_blend_pixel(img, row + x, col);
        }
}

LOC s32 s_sector_init(Sector *sec, f32 start, f32 end)
{
        f32 sweep = end - start;

        /*
         * Sweeps within S_ANGLE_EPS of a full turn count as a full circle,
         * and within S_ANGLE_EPS of zero as empty. Otherwise float rounding
         * could make e.g. pi .. 3pi "almost full" and 3pi .. pi "almost
         * empty", both with the same edge.
         */
        if(sweep >= S_TWO_PI - S_ANGLE_EPS) {
                return 1;
        }

        /* Wrap into [0, 2pi), so an end before the start goes around */
        sweep = fmodf(sweep, S_TWO_PI);
        if(sweep < 0.0f) {
                sweep += S_TWO_PI;
        }
        if(sweep < S_ANGLE_EPS) {
                return -1;
        }
        if(sweep > S_TWO_PI - S_ANGLE_EPS) {
                return 1;
        }

        /*
         * The end direction comes from end itself, not start + sweep, so two
         * arcs that meet at an angle use exactly the same edge.
         */
        sec->sx = cosf(start);
        sec->sy = sinf(start);
        sec->ex = cosf(end);
        sec->ey = sinf(end);

        sec->sx = SNAP_ZERO(sec->sx, S_DIR_EPS);
        sec->sy = SNAP_ZERO(sec->sy, S_DIR_EPS);
        sec->ex = SNAP_ZERO(sec->ex, S_DIR_EPS);
        sec->ey = SNAP_ZERO(sec->ey, S_DIR_EPS);

        sec->wide = sweep > S_PI;
        return 0;
}

LOC s32 s_sector_contains(const Sector *sec, s32 dx, s32 dy)
{
        f32 px = (f32)dx;
        f32 py = (f32)dy;

        /* >= 0: p is clockwise of start, > 0: p is counterclockwise of end */
        f32 after_start = sec->sx * py - sec->sy * px;
        f32 before_end = px * sec->ey - py * sec->ex;

        if(dx == 0 && dy == 0) {
                return 1;
        }

        /*
         * Pixels exactly on the line through the start or end edge are
         * decided here, because near a sweep of pi the general test below
         * depends on float rounding of the sweep:
         *
         * - on the start edge itself: inside
         * - on the end edge itself: outside
         * - on the extension behind the center: inside if it is strictly on
         *   the inner side of the other edge
         */
        if(after_start == 0.0f) {
                if(px * sec->sx + py * sec->sy > 0.0f) {
                        return 1;
                }
                return before_end > 0.0f;
        }
        if(before_end == 0.0f) {
                if(px * sec->ex + py * sec->ey > 0.0f) {
                        return 0;
                }
                return after_start > 0.0f;
        }

        /*
         * Up to half a circle the point has to be on the inner side of both
         * edges. For wider sectors it only has to be outside the remaining
         * narrow gap, so one edge is enough.
         */
        if(sec->wide) {
                return after_start >= 0.0f || before_end > 0.0f;
        }
        return after_start >= 0.0f && before_end > 0.0f;
}

LOC void s_arc_span(Image *img, Circle circ, const Sector *sec, s32 y,
                s32 x0, s32 x1, IColor col)
{
        s32 row;
        s32 x;

        if(!sec) {
                s_blend_span(img, y, x0, x1, col);
                return;
        }

        if(y < 0 || y >= img->height) {
                return;
        }

        x0 = MAX(x0, 0);
        x1 = MIN(x1, img->width - 1);
        row = y * img->width;

        for(x = x0; x <= x1; x++) {
                if(s_sector_contains(sec, x - circ.x, y - circ.y)) {
                        s_blend_pixel(img, row + x, col);
                }
        }
}

LOC void s_ring_row(Image *img, Circle circ, const Sector *sec, s32 y,
                s32 outer, s32 hole, IColor col)
{
        if(hole < 0) {
                s_arc_span(img, circ, sec, y, circ.x - outer, circ.x + outer,
                                col);
        } else if(hole < outer) {
                s_arc_span(img, circ, sec, y, circ.x - outer,
                                circ.x - hole - 1, col);
                s_arc_span(img, circ, sec, y, circ.x + hole + 1,
                                circ.x + outer, col);
        }
}

LOC void s_fill_circle(Image *img, Circle circ, const Sector *sec,
                IColor col)
{
        /*
         * Fill every pixel within r + 0.5 of the center, the same outer edge
         * as a 1 pixel outline. Because dx and dy are whole numbers,
         * dx² + dy² <= (r + 0.5)² is the same as dx² + dy² <= r² + r.
         */
        s64 limit = (s64)circ.r * circ.r + circ.r;
        s32 half = circ.r;
        s32 dy;

        if(circ.r < 0) {
                return;
        }

        /*
         * Go from the middle row outwards. The circle only gets narrower, so
         * the half width of the span just shrinks until it fits again.
         */
        for(dy = 0; dy <= circ.r; dy++) {
                s64 dy_sqr = (s64)dy * dy;

                while(half >= 0 && (s64)half * half + dy_sqr > limit) {
                        half--;
                }

                s_arc_span(img, circ, sec, circ.y - dy, circ.x - half,
                                circ.x + half, col);
                if(dy > 0) {
                        s_arc_span(img, circ, sec, circ.y + dy, circ.x - half,
                                        circ.x + half, col);
                }
        }
}

LOC void s_draw_circle(Image *img, Circle circ, const Sector *sec,
                IColor col, s32 w)
{
        /*
         * The outline is centered on the radius, so a pixel belongs to it if
         * r - w / 2 < distance <= r + w / 2. Multiplied by 2 and squared,
         * everything stays a whole number:
         *
         *   4 * (dx² + dy²) <= (2r + w)²   inside the outer edge
         *   4 * (dx² + dy²) >  (2r - w)²   outside the hole
         */
        s64 out_limit = (s64)(2 * circ.r + w) * (2 * circ.r + w);
        s64 in_limit = (s64)(2 * circ.r - w) * (2 * circ.r - w);
        s32 has_hole = 2 * circ.r - w > 0;
        s32 outer = circ.r + w;
        s32 hole = circ.r;
        s32 dy;

        if(w <= 0 || circ.r < 0) {
                return;
        }

        /*
         * Go from the middle row outwards. Both edges only move inwards, so
         * outer (last pixel of the ring) and hole (last pixel of the hole)
         * just shrink until they fit again.
         */
        for(dy = 0; dy <= circ.r + w; dy++) {
                s64 dy_sqr = (s64)dy * dy;

                while(outer >= 0 && 4 * ((s64)outer * outer + dy_sqr) >
                                out_limit) {
                        outer--;
                }
                if(outer < 0) {
                        break;
                }

                if(!has_hole) {
                        hole = -1;
                }
                while(hole >= 0 && 4 * ((s64)hole * hole + dy_sqr) >
                                in_limit) {
                        hole--;
                }

                s_ring_row(img, circ, sec, circ.y - dy, outer, hole, col);
                if(dy > 0) {
                        s_ring_row(img, circ, sec, circ.y + dy, outer, hole,
                                        col);
                }
        }
}

LOC s32 s_poly_prepare(Polygon *poly, Point **pts, void **scratch)
{
        s32 n = poly_count(poly);
        size_t per_edge = MAX(sizeof(Crossing), sizeof(Span));
        s32 i;

        if(n < 1) {
                return 0;
        }

        if(!(*pts = malloc(n * (sizeof(Point) + per_edge)))) {
                LOG(LOG_ERROR, "Failed to allocate memory for polygon");
                return -1;
        }
        *scratch = (u8 *)*pts + n * sizeof(Point);

        /* Copy once, so the rows don't go through the list every time */
        for(i = 0; i < n; i++) {
                (*pts)[i] = *(Point *)list_get(&poly->points, i);
        }

        return n;
}

LOC s32 s_capsule_row(Point a, Point b, f32 r, f32 sy, f32 *left,
                f32 *right)
{
        /*
         * The area within r of a segment (a capsule) is convex, so a row
         * crosses it in one range. The capsule is made of a circle around
         * each end and a band along the segment, so the range goes from the
         * leftmost to the rightmost point of the three parts.
         */
        f32 lo = 0.0f;
        f32 hi = 0.0f;
        s32 hit = 0;
        f32 dx = (f32)(b.x - a.x);
        f32 dy = (f32)(b.y - a.y);
        f32 len = sqrtf(dx * dx + dy * dy);
        Point ends[2];
        s32 i;

        ends[0] = a;
        ends[1] = b;

        /* The circles around both ends */
        for(i = 0; i < 2; i++) {
                f32 ey = sy - ends[i].y;
                f32 h;

                /* A little slack, the exact test trims the range later */
                if(ey * ey > r * r + 0.01f) {
                        continue;
                }

                h = sqrtf(MAX(r * r - ey * ey, 0.0f));
                if(!hit || ends[i].x - h < lo) lo = ends[i].x - h;
                if(!hit || ends[i].x + h > hi) hi = ends[i].x + h;
                hit = 1;
        }

        /*
         * The band: a parallelogram with the corners a and b moved by r
         * along the normal in both directions. Check where the row crosses
         * its four sides.
         */
        if(len > 0.0f) {
                f32 nx = -dy / len * r;
                f32 ny = dx / len * r;
                f32 qx[4];
                f32 qy[4];

                qx[0] = a.x + nx;  qy[0] = a.y + ny;
                qx[1] = b.x + nx;  qy[1] = b.y + ny;
                qx[2] = b.x - nx;  qy[2] = b.y - ny;
                qx[3] = a.x - nx;  qy[3] = a.y - ny;

                for(i = 0; i < 4; i++) {
                        s32 j = (i + 1) % 4;
                        f32 x;

                        if((sy < qy[i]) == (sy < qy[j]) && sy != qy[i]) {
                                continue;
                        }

                        if(qy[i] == qy[j]) {
                                /* Side lies on the row: both ends count */
                                x = MIN(qx[i], qx[j]);
                                if(!hit || x < lo) lo = x;
                                x = MAX(qx[i], qx[j]);
                                if(!hit || x > hi) hi = x;
                        } else {
                                x = qx[i] + (sy - qy[i]) * (qx[j] - qx[i]) /
                                        (qy[j] - qy[i]);
                                if(!hit || x < lo) lo = x;
                                if(!hit || x > hi) hi = x;
                        }
                        hit = 1;
                }
        }

        /* A little slack, the exact test trims the range later */
        *left = lo - 0.01f;
        *right = hi + 0.01f;
        return hit;
}

LOC s32 s_in_capsule(Point a, Point b, s32 w, s32 x, s32 y)
{
        /*
         * Double all coordinates, so the pixel center (x + 0.5) becomes the
         * whole number 2x + 1 and the radius w / 2 becomes w. Then every
         * value below is a whole number and the comparisons are exact.
         */
        s64 dx = 2 * (s64)(b.x - a.x);
        s64 dy = 2 * (s64)(b.y - a.y);
        s64 vx = 2 * (s64)x + 1 - 2 * (s64)a.x;
        s64 vy = 2 * (s64)y + 1 - 2 * (s64)a.y;
        s64 r_sqr = (s64)w * w;
        s64 len_sqr = dx * dx + dy * dy;
        s64 dot = vx * dx + vy * dy;
        s64 cross;

        /* Closest to a: compare the distance to a */
        if(len_sqr == 0 || dot <= 0) {
                return vx * vx + vy * vy <= r_sqr;
        }

        /* Closest to b: compare the distance to b */
        if(dot >= len_sqr) {
                s64 ux = vx - dx;
                s64 uy = vy - dy;
                return ux * ux + uy * uy <= r_sqr;
        }

        /*
         * Next to the segment: the distance is |cross| / |d|, so compare
         * cross² <= r² * |d|² to avoid the division.
         */
        cross = vx * dy - vy * dx;
        return cross * cross <= r_sqr * len_sqr;
}
