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
#include "png.h"

#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wpadded"
#endif
#include <png.h>


/*
 * Rationale:
 * 
 *   Users want their files in PNG (most often), not PNM.
 *   Calling an external program for conversion poses
 *   potential security risks, and is slower.
 */


/**
 * Convert an image from PNM to PNG.
 * 
 * This is a very limited conversion implementation.
 * It is not generally reusable. It makes assumptions
 * that are true only necessarily for this program.
 * 
 * @param   fdin   The file descriptor for the image to convert (PNM).
 * @param   fdout  The file descriptor for the output image (PNG).
 * @return         Zero on success, -1 on error.
 */
int
convert (int fdin, int fdout)
{
#define SCAN(...)  do { if (fscanf(in, __VA_ARGS__) != 1)  goto fail; } while (0)

  FILE *in = NULL;
  FILE *out = NULL;
  long height, width, width3;
  long y, x, r, g, b;
  png_byte   *pixbuf = 0;
  png_struct *pngbuf = NULL;
  png_info   *pnginfo = NULL;
  int saved_errno = 0, rc, c;
  
  /* Get a FILE * for the input, we want to use fscanf, there is no dscanf. */
  in = fdopen (fdin, "r");
  if (in == NULL)
    goto fail;
  /* Get a FILE * for the output, libpng wants a FILE *, not a file descriptor. */
  out = fdopen (fdout, "w");
  if (out == NULL)
    goto fail;
  
  /* Read head. */
  do c = fgetc(in); while ((c != '\n') && (c != EOF));
  SCAN ("%li ", &width);
  SCAN ("%li\n", &height);
  do c = fgetc(in); while ((c != '\n') && (c != EOF));
  
  /* Allocte structures for the PNG. */
  width3 = width * 3;
  pixbuf = malloc ((size_t)width3 * sizeof (png_byte));
  if (pixbuf == NULL)
    goto fail;
  pngbuf = png_create_write_struct (png_get_libpng_ver (NULL), NULL, NULL, NULL);
  if (pngbuf == NULL)
    goto fail;
  pnginfo = png_create_info_struct (pngbuf);
  if (pnginfo == NULL)
    goto fail;
  
  /* Initialise PNG write, and write head. */
  if (setjmp (png_jmpbuf(pngbuf))) /* Failing libpng calls jump here. */
    goto fail;
  png_init_io (pngbuf, out);
  png_set_IHDR (pngbuf, pnginfo, (png_uint_32)width, (png_uint_32)height,
		8 /* bit depth */, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info (pngbuf, pnginfo);
  
  /* Read and convert body. */
  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width3; x += 3)
	{
	  SCAN ("%li\n", &r);
	  SCAN ("%li\n", &g);
	  SCAN ("%li\n", &b);
	  pixbuf[x + 0] = (png_byte)r;
	  pixbuf[x + 1] = (png_byte)g;
	  pixbuf[x + 2] = (png_byte)b;
	}
      png_write_row (pngbuf, pixbuf);
    }
  png_write_end (pngbuf, pnginfo);
  
  rc = 0;
  goto cleanup;
 fail:
  saved_errno = errno;
  rc = -1;
  goto cleanup;
  
 cleanup: 
  png_destroy_write_struct (&pngbuf, (pnginfo ? &pnginfo : NULL));
  if (in != NULL)
    fclose (in);
  if (out != NULL)
    fclose (out);
  free (pixbuf);
  return errno = saved_errno, rc;
}

