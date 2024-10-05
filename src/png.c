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
#include "png.h"
#include "kern.h"


/*
 * Rationale:
 * 
 *   Users want their files in PNG (most often), not PNM.
 *   Calling an external program for conversion poses
 *   potential security risks, and is significantly slower.
 */


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
save_png (int fbfd, long width, long height, int imgfd, void *restrict data)
{
  char buf[8 << 10];
  FILE *file = NULL;
  ssize_t got, off;
  size_t adjustment;
  png_byte   *restrict pixbuf = NULL;
  png_struct *pngbuf = NULL;
  png_info   *pnginfo = NULL;
  long width3, state = 0;
  int rc, saved_errno;
  
  /* Get a FILE * for the output, libpng wants a FILE *, not a file descriptor. */
  file = fdopen (imgfd, "w");
  if (file == NULL)
    goto fail;
  
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
  errno = 0;
  if (setjmp (png_jmpbuf(pngbuf))) /* Failing libpng calls jump here. */
    goto fail;
  png_init_io (pngbuf, file);
  png_set_IHDR (pngbuf, pnginfo, (png_uint_32)width, (png_uint_32)height,
		8 /* bit depth */, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info (pngbuf, pnginfo);
  
  /* TODO (maybe) The image shall be packed. That is, if 24 bits per pixel is
   *              unnecessary, less shall be used. 6 bits is often sufficient. */
  
  /* Convert raw framebuffer data into a PNG image (body). */
  for (off = 0;;)
    {
      /* Read data from the framebuffer, we may have up to 3 bytes buffered. */
      got = read (fbfd, buf + off, sizeof (buf) - (size_t)off * sizeof (char));
      if (got < 0)
	goto fail;
      if (got == 0)
	break;
      got += off;
      
      /* Convert read pixels. */
      if (convert_fb_to_png (pngbuf, pixbuf, buf, (size_t)got,
			     width3, &adjustment, &state, data) < 0)
	goto fail;
      
      /* If we read a whole number of pixels, reset the buffer, otherwise,
         move the unconverted bytes to the beginning of the buffer. */
      if (adjustment)
	{
	  off -= (ssize_t)adjustment;
	  memcpy (buf, buf + off, (size_t)(got - off) * sizeof (char));
	  off = got - off;
	}
      else
	off = 0;
    }
  
  /* Done! */
  png_write_end (pngbuf, pnginfo);
  rc = 0;
  goto cleanup;
 fail:
  saved_errno = errno;
  rc = -1;
  goto cleanup;
  
 cleanup: 
  png_destroy_write_struct (&pngbuf, (pnginfo ? &pnginfo : NULL));
  if (file != NULL)
    {
      fflush (file);
      fclose (file);
    }
  free (pixbuf);
  errno = saved_errno;
  return rc;
}

