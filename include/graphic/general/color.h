#ifndef VASALL_COLOR_H
#define VASALL_COLOR_H

#include <devkit/scope.h>
#include <devkit/types.h>

/******************************************************************************
 *
 * 				    ICOLOR
 *
 ******************************************************************************/

typedef struct IColor {
        u8 r;
        u8 g;
        u8 b;
        u8 a;
} IColor;

PUB IColor icolor(u8 r, u8 g, u8 b, u8 a);

/*
 * Convert a color to premultiplied alpha by multiplying the RGB channels
 * with the alpha value.
 *
 * @param col The color with straight alpha
 *
 * @returns The color with premultiplied alpha
*/
PUB IColor icolor_premultiply(IColor col);

/*
 * Blend a color over another color ("over" operation).
 *
 * @param dst The current color, with premultiplied alpha
 * @param src The new color to draw on top, with straight alpha
 *
 * @returns The blended color, with premultiplied alpha
*/
PUB IColor icolor_blend(IColor dst, IColor src);

/*
 * Blend a color over another color ("over" operation), when both colors
 * already use premultiplied alpha, e.g. when copying between images.
 *
 * @param dst The current color, with premultiplied alpha
 * @param src The new color to draw on top, with premultiplied alpha
 *
 * @returns The blended color, with premultiplied alpha
*/
PUB IColor icolor_blend_premul(IColor dst, IColor src);

/******************************************************************************
 *
 * 				    FCOLOR
 *
 ******************************************************************************/

typedef struct FColor {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
} FColor;

PUB FColor fcolor(f32 r, f32 g, f32 b, f32 a);
PUB FColor fcolor_from_u8(IColor col);

#endif /* VASALL_COLOR_H */