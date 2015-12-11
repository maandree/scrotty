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
#include "common.h"
#include "kern.h"
#include "pnm.h"
#include "png.h"



/**
 * Stop when `try_alt_fbpath` (in scrotty.c) reaches this value.
 */
const int alt_fbpath_limit = 2;



/**
 * This is called if no framebuffer is found.
 * 
 * It prints a message describing this condition
 * and suggests how to resolve it.
 */
void
print_not_found_help (void)
{
  fprintf (stderr, _("%s: Unable to find a framebuffer. "
		     "If you have a file named %s/MAKEDEV, "
		     "run it with '-d /dev/fb' as root, "
		     "or try running '%s'.\n"),
	   execname, DEVDIR, "mknod /dev/fb0 c 29 0 && chgrp video /dev/fb0");
}


/**
 * Construct the path to a framebuffer device.
 * 
 * @param   altpath  The index of the alternative path-pattern to use.
 * @param   fbno     The index of the framebuffer.
 * @return           The path to the framebuffer device. Errors are impossible.
 *                   This string is statically allocated and must not be deallocated.
 */
char *
get_fbpath (int altpath, int fbno)
{
  static char pathbuf[sizeof (DEVDIR "/fb/") + 3 * sizeof (int)];
  sprintf (pathbuf, "%s/fb%s%i", DEVDIR, (altpath ? "/" : ""), fbno);
  return pathbuf;
}


/**
 * Get the dimensions of a framebuffer.
 * 
 * @param   fbno    The number of the framebuffer.
 * @param   fbpath  The path to the framebuffer device..
 * @param   width   Output parameter for the width of the image.
 * @param   height  Output parameter for the height of the image.
 * @return          Zero on success, -1 on error.
 */
int
measure (int fbno, char *restrict fbpath, long *restrict width, long *restrict height)
{
  static char buf[sizeof (SYSDIR "/class/graphics/fb/virtual_size") + 3 * sizeof(int)];
  /* The string "/class/graphics/fb/virtual_size" is large enought for the call (*) "*/
  char *delim;
  int sizefd = -1;
  ssize_t got;
  int saved_errno;
  
  /* Open the file with the framebuffer's dimensions. */
  sprintf (buf, "%s/class/graphics/fb%i/virtual_size", SYSDIR, fbno);
  sizefd = open (buf, O_RDONLY);
  if (sizefd == -1)
    FILE_FAILURE (buf);
  
  /* Get the dimensions of the framebuffer. */
  got = read (sizefd, buf, sizeof (buf) / sizeof (char) - 1); /* (*) */
  if (got < 0)
    goto fail;
  close (sizefd);
  
  /* The read content is formated as `%{width},%{height}\n`,
     convert it to `%{width}\0%{height}\0` and parse it. */
  buf[got] = '\0';
  delim = strchr (buf, ',');
  *delim++ = '\0';
  *width = atol (buf);
  *height = atol (delim);
  
  return 0;
  
 fail:
  saved_errno = errno;
  if (sizefd >= 0)
    close (sizefd);
  errno = saved_errno;
  return -1;
  
  (void) fbpath;
}


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
int
convert_fb_to_pnm (FILE *restrict file, const char *restrict buf,
		   size_t n, size_t *restrict adjustment)
{
  const uint32_t *restrict pixel;
  int r, g, b;
  size_t off;
  
  for (off = 0; off < n; off += 4)
    {
      /* A pixel in the framebuffer is formatted as `%{blue}%{green}%{red}%{x}`
	 in big-endian binary, or `%{x}%{red}%{green}%{blue}` in little-endian binary. */
      pixel = (const uint32_t *)(buf + off);
      r = (*pixel >> 16) & 255;
      g = (*pixel >> 8) & 255;
      b = (*pixel >> 0) & 255;
      
      if (SAVE_PNM_PIXEL (file, r, g, b) < 0)
	goto fail;
    }
  
  *adjustment = (off != n ? 4 : 0);
  return 0;
 fail:
  return -1;
}


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
 * @return              Zero on success, -1 on error.
 */
int convert_fb_to_png (png_struct *pngbuf, png_byte *restrict pixbuf, const char *restrict buf,
		       size_t n, long width3, size_t *restrict adjustment, long *restrict state)
{
  const uint32_t *restrict pixel;
  int r, g, b;
  size_t off;
  long x3 = *state;
  
  for (off = 0; off < n; off += 4)
    {
      /* A pixel in the framebuffer is formatted as `%{blue}%{green}%{red}%{x}`
	 in big-endian binary, or `%{x}%{red}%{green}%{blue}` in little-endian binary. */
      pixel = (const uint32_t *)(buf + off);
      r = (*pixel >> 16) & 255;
      g = (*pixel >> 8) & 255;
      b = (*pixel >> 0) & 255;
      
      SAVE_PNG_PIXEL (pixbuf, x3, r, g, b);
      x3 += 3;
      if (x3 == width3)
	{
	  SAVE_PNG_ROW (pngbuf, pixbuf);
	  x3 = 0;
	}
    }
  
  *adjustment = (off != n ? 4 : 0);
  *state = x3;
  return 0;
}

