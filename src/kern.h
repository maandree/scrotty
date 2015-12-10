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
 * @param  pathbuf  Ouput buffer for the path.
 * @param  altpath  The index of the alternative path-pattern to use.
 * @param  fbno     The index of the framebuffer.
 */
void get_fbpath (char *pathbuf, int altpath, int fbno);

/**
 * Get the dimensions of a framebuffer.
 * 
 * @param   fbno    The number of the framebuffer.
 * @param   fbpath  The path to the framebuffer device..
 * @param   width   Output parameter for the width of the image.
 * @param   height  Output parameter for the height of the image.
 * @return          Zero on success, -1 on error.
 */
int measure (int fbno, char *fbpath, long *width, long *height);

/**
 * Convert read data from a framebuffer to PNM pixel data.
 * 
 * @param   file        The output image file.
 * @param   buf         Buffer with read data.
 * @param   n           The number of read characters.
 * @param   adjustment  Set to zero if all bytes were converted
 *                      (a whole number of pixels where available,)
 *                      otherwise, set to the number of bytes a
 *                      pixel is encoded.
 * @return              Zero on success, -1 on error.
 */
int convert_fb (FILE *file, char *buf, size_t n, size_t *adjustment);

