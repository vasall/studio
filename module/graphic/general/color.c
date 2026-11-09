#include <graphic/general/color.h>

/******************************************************************************
 *
 * 				    ICOLOR
 *
 ******************************************************************************/

PUB IColor icolor(u8 r, u8 g, u8 b, u8 a)
{
        IColor col;
        col.r = r;
        col.g = g;
        col.b = b;
        col.a = a;
        return col;
}

PUB IColor icolor_premultiply(IColor col)
{
        /* +127 rounds instead of truncating */
        col.r = (u8)((col.r * col.a + 127) / 255);
        col.g = (u8)((col.g * col.a + 127) / 255);
        col.b = (u8)((col.b * col.a + 127) / 255);
        return col;
}

PUB IColor icolor_blend(IColor dst, IColor src)
{
        return icolor_blend_premul(dst, icolor_premultiply(src));
}

PUB IColor icolor_blend_premul(IColor dst, IColor src)
{
        u32 ia = 255 - src.a;
        IColor out;

        /* Fully opaque or fully transparent: no math needed */
        if(src.a == 255) return src;
        if(src.a == 0)   return dst;

        /* result = src + dst * (1 - src_alpha), for all four channels */
        out.r = (u8)(src.r + (dst.r * ia + 127) / 255);
        out.g = (u8)(src.g + (dst.g * ia + 127) / 255);
        out.b = (u8)(src.b + (dst.b * ia + 127) / 255);
        out.a = (u8)(src.a + (dst.a * ia + 127) / 255);
        return out;
}

/******************************************************************************
 *
 * 				    FCOLOR
 *
 ******************************************************************************/

PUB FColor fcolor(f32 r, f32 g, f32 b, f32 a)
{
        FColor fcol;
        fcol.r = r;
        fcol.g = g;
        fcol.b = b;
        fcol.a = a;
        return fcol;
}

PUB FColor fcolor_from_u8(IColor col)
{
        FColor fcol;
        fcol.r = (f32)col.r / 255.0f;
        fcol.g = (f32)col.g / 255.0f;
        fcol.b = (f32)col.b / 255.0f;
        fcol.a = (f32)col.a / 255.0f;
        return fcol;
}