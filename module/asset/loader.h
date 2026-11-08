#ifndef INTERNAL_VASALL_LOADER_H
#define INTERNAL_VASALL_LOADER_H

/*
 * Loaders turn files into graphic and devkit types. They are internal to the
 * asset module and don't know about the asset cache.
 */

#include <devkit/scope.h>
#include <devkit/types.h>
#include <devkit/memory/text.h>

#include <graphic/image.h>
#include <graphic/font.h>

/******************************************************************************
 *
 * 				PRIVATE INTERFACE
 *
 ******************************************************************************/

/*
 * Read a whole file into memory. Implemented per platform, e.g. in
 * loader_file_linux.c.
 *
 * @param path The path to the file
 * @param data_out Receives the file contents, free it with free()
 * @param size_out Receives the number of bytes
 *
 * @returns 0 on success, -1 if an error occurred
 */
PRV s32 load_file(const char *path, u8 **data_out, s32 *size_out);

/*
 * Load an image file (PNG, JPG, BMP, TGA, GIF, PSD, HDR, PIC, PNM). The
 * pixels are converted to RGBA with premultiplied alpha, like all images.
 *
 * @param path The path to the file
 * @param img Receives the image, free it with img_deinit
 *
 * @returns 0 on success, -1 if an error occurred
 */
PRV s32 load_image(const char *path, Image *img);

/*
 * Load a TrueType or OpenType font file (.ttf, .otf).
 *
 * @param path The path to the file
 * @param font Receives the font, free it with font_deinit
 *
 * @returns 0 on success, -1 if an error occurred
 */
PRV s32 load_font(const char *path, Font *font);

/*
 * Load a UTF-8 text file, e.g. shader source. A UTF-8 byte order mark at the
 * start is removed.
 *
 * @param path The path to the file
 * @param txt Receives the text, free it with text_deinit
 *
 * @returns 0 on success, -1 if an error occurred or the file is not valid
 *          UTF-8
 */
PRV s32 load_text(const char *path, Text *txt);

#endif /* INTERNAL_VASALL_LOADER_H */
