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
 * @param  pathbuf  Ouput buffer for the path.
 * @param  altpath  The index of the alternative path-pattern to use.
 * @param  fbno     The index of the framebuffer.
 */
void
get_fbpath (char *pathbuf, int altpath, int fbno)
{
  sprintf (pathbuf, "%s/fb%s%i", DEVDIR, (altpath ? "/" : ""), fbno);
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
measure (int fbno, char *fbpath, long *width, long *height)
{
  static char buf[PATH_MAX];
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
  got = read (sizefd, buf, sizeof (buf) / sizeof (char) - 1);
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
convert_fb (FILE *file, char *buf, size_t n, size_t *adjustment)
{
  uint32_t *pixel;
  int r, g, b;
  size_t off;
  
  for (off = 0; off < n; off += 4)
    {
      /* A pixel in the framebuffer is formatted as `%{blue}%{green}%{red}%{x}`
	 in big-endian binary, or `%{x}%{red}%{green}%{blue}` in little-endian binary. */
      pixel = (uint32_t *)(buf + off);
      r = (*pixel >> 16) & 255;
      g = (*pixel >> 8) & 255;
      b = (*pixel >> 0) & 255;
      
      if (SAVE_PIXEL(file, r, g, b) < 0)
	goto fail;
    }
  
  *adjustment = (off != n ? 4 : 0);
  return 0;
 fail:
  return -1;
}

