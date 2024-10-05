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
#include <stddef.h>


/**
 * Parse and evaluate a --exec argument or filename pattern.
 * 
 * If `path != NULL` than all non-escaped spaces in
 * `pattern` will be stored as 255-bytes in `buf`.
 * 
 * @param   pattern  The pattern to evaluate.
 * @param   fbno     The index of the framebuffer.
 * @param   width    The width of the image/framebuffer.
 * @param   height   The height of the image/framebuffer.
 * @param   path     The filename of the saved image, `NULL`
 *                   during the evaluation of the filename pattern.
 * @return           The constructed string, `NULL` on error.
 */
char *evaluate (const char *restrict pattern, int fbno, long width,
		long height, const char *restrict path);

