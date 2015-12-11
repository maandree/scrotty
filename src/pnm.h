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
 * Store a pixel to a PNM image.
 * 
 * A pixel in the PNM image is formatted as `%{red} %{green} %{blue} ` in text.
 * 
 * @param   F:FILE *  The file whither the pixel shall be save.
 * @param   R:int     The [0, 255]-value on the red subpixel.
 * @param   G:int     The [0, 255]-value on the green subpixel.
 * @param   B:int     The [0, 255]-value on the blue subpixel.
 * @return            Positive on success (not zero!), -1 on error.
 */
#define SAVE_PNM_PIXEL(F, R, G, B)  \
  fprintf (F, "%s%s%s", inttable[R], inttable[G], inttable[B])


/**
 * [0, 255]-integer-to-text convertion lookup table for faster conversion from
 * raw framebuffer data to the PNM format. The values have a whitespace at the
 * end for even faster conversion.
 * Lines should not be longer than 70 (although most programs will probably
 * work even if there are longer lines), therefore the selected whitespace
 * is LF (new line).
 * 
 * ASCII is wider supported than binary, and is create for version control,
 * especifially with one datum per line.
 */
extern const char* inttable[];


/**
 * Create an PNM file.
 * 
 * @param   fbfd    The file descriptor connected to framebuffer device.
 * @param   width   The width of the image.
 * @param   height  The height of the image.
 * @param   imgfd   The file descriptor connected to conversion process's stdin.
 * @param   data    Additional data for `convert_fb_to_pnm`
 *                  and `convert_fb_to_png`.
 * @return          Zero on success, -1 on error.
 */
int save_pnm (int fbfd, long width, long height, int imgfd, void *restrict data);

