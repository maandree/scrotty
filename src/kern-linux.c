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
#include "common.h"
#include "kern.h"
#include "png.h"

#include <sys/ioctl.h>
#include <linux/fb.h>



/**
 * Stop when `try_alt_fbpath` (in scrotty.c) reaches this value.
 */
const int alt_fbpath_limit = 2;

/**
 * Addition metadata for the framebuffer.
 */
struct data
{
  /**
   * The number dead pixels at the beginning.
   */
  unsigned long start;
  
  /**
   * The byte where the frame end. Zero if
   * there is neither panning or padding.
   */
  unsigned long end;
  
  /**
   * The number of dead pixels between lines.
   */
  long hblank;
  
  /**
   * This variable is used to keep track of
   * how many pixels have been read.
   */
  unsigned long position;
};



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
 * @param   fbfd    File descriptor for framebuffer device.
 * @param   width   Output parameter for the width of the image.
 * @param   height  Output parameter for the height of the image.
 * @parma   data    Additional data to pass to `convert_fb_to_png`.
 * @return          Zero on success, -1 on error.
 */
int
measure (int fbno, int fbfd, long *restrict width, long *restrict height, void **restrict data)
{
  static struct data d;
  struct fb_fix_screeninfo fixinfo;
  struct fb_var_screeninfo varinfo;
  unsigned long int linelength;
  
  /* Get configurations. */
  if (ioctl (fbfd, FBIOGET_FSCREENINFO, &fixinfo))
    goto fail;
  if (ioctl (fbfd, FBIOGET_VSCREENINFO, &varinfo))
    goto fail;
  
  /* Get dimension. */
  *width  = varinfo.xres;
  *height = varinfo.yres;
  
  /* Are the configurations supported? */
  if (varinfo.bits_per_pixel & 7)
    {
      fprintf(stderr, _("%s: Unsupported framebuffer configurations: "
			"pixels are not encoded in whole bytes.\n"), execname);
      exit(1);
    }
  
  /* Get dead area information. */
  linelength = fixinfo.line_length / (varinfo.bits_per_pixel / 8);
  d.start = varinfo.yoffset * linelength;
  d.start += varinfo.xoffset;
  d.end = d.start + linelength * varinfo.yres;
  if ((d.start == 0) && (linelength == 0))
    d.end = 0;
  d.hblank = linelength - *width;
  d.position = 0;
  
  /* TODO depth support */
  
  *data = &d;
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
 * @param   data        Data from `measure`.
 * @return              Zero on success, -1 on error.
 */
int
convert_fb_to_png (png_struct *pngbuf, png_byte *restrict pixbuf, const char *restrict buf, size_t n,
		   long width3, size_t *restrict adjustment, long *restrict state, void *restrict data)
{
#define STORE(PADDING_AND_PANNING)			\
  do							\
    {							\
      if (PADDING_AND_PANNING)				\
	if ((pos < d.start) || (pos >= d.end))		\
	  continue;					\
							\
      if (PADDING_AND_PANNING ? (x3 < width3) : 1)	\
	SAVE_PNG_PIXEL (pixbuf, x3, r, g, b);		\
      x3 += 3;						\
      if (x3 == lineend)				\
	{						\
	  SAVE_PNG_ROW (pngbuf, pixbuf);		\
	  x3 = 0;					\
	}						\
    }							\
  while (0)
  
  const uint32_t *restrict pixel;
  int r, g, b;
  size_t off;
  long x3 = *state;
  struct data d = *(struct data *)data;
  unsigned long pos = d.position;
  long lineend = width3 + d.hblank * 3;
  
  if ((d.start == 0) && (d.hblank == 0) && (d.end == 0)) /* Optimised version for customary settings. */
    for (off = 0; off < n; off += 4)
      {
	/* A pixel in the framebuffer is formatted as `%{blue}%{green}%{red}%{x}`
	   in big-endian binary, or `%{x}%{red}%{green}%{blue}` in little-endian binary. */
	pixel = (const uint32_t *)(buf + off);
	r = (*pixel >> 16) & 255;
	g = (*pixel >> 8) & 255;
	b = (*pixel >> 0) & 255;
	
	STORE(0);
      }
  else
    for (off = 0; off < n; off += 4, pos++)
      {
	/* A pixel in the framebuffer is formatted as `%{blue}%{green}%{red}%{x}`
	   in big-endian binary, or `%{x}%{red}%{green}%{blue}` in little-endian binary. */
	pixel = (const uint32_t *)(buf + off);
	r = (*pixel >> 16) & 255;
	g = (*pixel >> 8) & 255;
	b = (*pixel >> 0) & 255;
	
	STORE(1);
      }
  
  *adjustment = (off != n ? 4 : 0);
  *state = x3;
  ((struct data *)data)->position = pos;
  return 0;
}

