/**
 * scrotty — Screenshot program for Linux's TTY
 * 
 * Copyright © 2014, 2015  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wpadded"
#endif
#include <png.h>
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif



/**
 * Store a pixel to a PNG row buffer.
 * 
 * @param  PIXBUF:png_byte *  The pixel buffer for the row.
 * @param  X3:long            The column of the pixel multipled by 3.
 * @param  R:int              The [0, 255]-value on the red subpixel.
 * @param  G:int              The [0, 255]-value on the green subpixel.
 * @param  B:int              The [0, 255]-value on the blue subpixel.
 */
#define SAVE_PNG_PIXEL(PIXBUF, X3, R, G, B)	\
  ((PIXBUF)[(X3) + 0] = (png_byte)(R),		\
   (PIXBUF)[(X3) + 1] = (png_byte)(G),		\
   (PIXBUF)[(X3) + 2] = (png_byte)(B))

/**
 * Store a row to a PNG image.
 * 
 * @param  PNGBUF:png_struct *  The PNG image structure.
 * @param  PIXBUF:png_byte *    The pixel buffer for the row.
 */
#define SAVE_PNG_ROW(PNGBUF, PIXBUF)  \
  png_write_row (PNGBUF, PIXBUF)


/**
 * Create an PNG file.
 * 
 * @param   fbfd    The file descriptor connected to framebuffer device.
 * @param   width   The width of the image.
 * @param   height  The height of the image.
 * @param   imgfd   The file descriptor connected to conversion process's stdin.
 * @param   data    Additional data for `convert_fb_to_png`.
 * @return          Zero on success, -1 on error.
 */
int
save_png (int fbfd, long width, long height, int imgfd, void *restrict data);

