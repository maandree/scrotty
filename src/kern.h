/**
 * scrotty — Screenshot program for Linux's TTY
 * 
 * Copyright © 2014, 2015  Mattias Andrée (m@maandree.se)
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
 * Stop when `try_alt_fbpath` (in scrotty.c) reaches this value.
 */
extern const int alt_fbpath_limit;


/**
 * This is called if no framebuffer is found.
 * 
 * It prints a message describing this condition
 * and suggests how to resolve it.
 */
void print_not_found_help (void);

/**
 * Construct the path to a framebuffer device.
 * 
 * @param   altpath  The index of the alternative path-pattern to use.
 * @param   fbno     The index of the framebuffer.
 * @return           The path to the framebuffer device. Errors are impossible.
 *                   This string is statically allocated and must not be deallocated.
 */
char *get_fbpath (int altpath, int fbno);

/**
 * Get the dimensions of a framebuffer.
 * 
 * @param   fbno    The number of the framebuffer.
 * @param   fbfd    File descriptor for framebuffer device.
 * @param   width   Output parameter for the width of the image.
 * @param   height  Output parameter for the height of the image.
 * @parma   data    Additional data to pass to `convert_fb_to_png`.
 * @return          Zero on success, -1 on error.
 */
int measure (int fbno, int fbfd, long *restrict width, long *restrict height, void **restrict data);

/**
 * Convert read data from a framebuffer to PNG pixel data.
 * 
 * @param   file        The output image file.
 * @param   buf         Buffer with read data.
 * @param   n           The number of read characters.
 * @param   width3      The width of the image multipled by 3.
 * @param   adjustment  Set to zero if all bytes were converted
 *                      (a whole number of pixels where available,)
 *                      otherwise, set to the number of bytes a
 *                      pixel is encoded.
 * @param   state       Use this to keep track of where in the you
 *                      stopped. It will be 0 on the first call.
 * @param   data        Data from `measure`.
 * @return              Zero on success, -1 on error.
 */
int convert_fb_to_png (png_struct *pngbuf, png_byte *restrict pixbuf, const char *restrict buf,
		       size_t n, long width3, size_t *restrict adjustment, long *restrict state,
		       void *restrict data);

