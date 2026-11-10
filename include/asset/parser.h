#ifndef VASALL_PARSER_H
#define VASALL_PARSER_H

#include <devkit/scope.h>
#include <devkit/types.h>

#include <devkit/memory/text.h>

#include <graphic/image/image.h>
#include <graphic/font/font.h>

/*
 * Parsers turn the raw bytes of a file into graphic and devkit types. They
 * don't know where the bytes come from, so they also work on data that was
 * never a file.
 */

/******************************************************************************
 *
 * 				PUBLIC INTERFACE
 *
 ******************************************************************************/

/*
 * Decode an image (PNG, JPG, BMP, TGA, GIF, PSD, HDR, PIC, PNM). The pixels
 * are converted to RGBA with premultiplied alpha, like all images.
 *
 * @param data The file contents
 * @param size The number of bytes in data
 * @param img Receives the image, free it with img_deinit
 *
 * @returns 0 on success, -1 if the data is not a supported image
 */
PUB s32 parse_image(const u8 *data, s32 size, Image *img);

/*
 * Parse a TrueType or OpenType font (.ttf, .otf). The font keeps using the
 * data, so on success it takes ownership of it and frees it in font_deinit.
 * On failure data still belongs to the caller.
 *
 * @param data The file contents, allocated with malloc
 * @param size The number of bytes in data
 * @param font Receives the font, free it with font_deinit
 *
 * @returns 0 on success, -1 if the data is not a usable font
 */
PUB s32 parse_font(u8 *data, s32 size, Font *font);

/*
 * Copy UTF-8 text, e.g. shader source. A UTF-8 byte order mark at the start
 * is removed.
 *
 * @param data The file contents
 * @param size The number of bytes in data
 * @param txt Receives the text, free it with text_deinit
 *
 * @returns 0 on success, -1 if the data is not valid UTF-8
 */
PUB s32 parse_text(const u8 *data, s32 size, Text *txt);

#endif /* VASALL_PARSER_H */
