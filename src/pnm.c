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
#include "pnm.h"
#include "kern.h"



#define LIST_0_9(P)  P"0\n", P"1\n", P"2\n", P"3\n", P"4\n", P"5\n", P"6\n", P"7\n", P"8\n", P"9\n"
/**
 * [0, 255]-integer-to-text convertion lookup table for faster conversion from
 * raw framebuffer data to the PNM format. The values have a whitespace at the
 * end for even faster conversion.
 * Lines should not be longer than 70 (although most programs will probably
 * work even if there are longer lines), therefore the selected whitespace
 * is LF (new line).
 * 
 * ASCII has wider support than binary, and is create for version control,
 * especifially with one datum per line.
 */
const char* inttable[] =
  {
    LIST_0_9(""),  LIST_0_9("1"), LIST_0_9("2"), LIST_0_9("3"), LIST_0_9("4"),
    LIST_0_9("5"), LIST_0_9("6"), LIST_0_9("7"), LIST_0_9("8"), LIST_0_9("9"),
    
    LIST_0_9("10"), LIST_0_9("11"), LIST_0_9("12"), LIST_0_9("13"), LIST_0_9("14"),
    LIST_0_9("15"), LIST_0_9("16"), LIST_0_9("17"), LIST_0_9("18"), LIST_0_9("19"),
    
    LIST_0_9("20"), LIST_0_9("21"), LIST_0_9("22"), LIST_0_9("23"), LIST_0_9("24"),
    "250\n", "251\n", "252\n", "253\n", "254\n", "255\n"
  };



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
int
save_pnm (int fbfd, long width, long height, int imgfd, void *restrict data)
{
  char buf[8 << 10];
  FILE *file = NULL;
  ssize_t got, off;
  size_t adjustment;
  int saved_errno;
  
  /* Create a FILE *, for writing, for the image file. */
  file = fdopen (imgfd, "w");
  if (file == NULL)
    goto fail;
  
  /* The PNM image should begin with `P3\n%{width} %{height}\n%{colour max=255}\n`.
     ('\n' and ' ' can be exchanged at will.) */
  if (fprintf (file, "P3\n%li %li\n255\n", width, height) < 0)
    goto fail;
  
  /* Convert raw framebuffer data into a PNM image. */
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
      if (convert_fb_to_pnm (file, buf, (size_t)got, &adjustment, data) < 0)
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
  
  /* Close file and return successfully. */
  fflush (file);
  fclose (file);
  return 0;
  
 fail:
  saved_errno = errno;
  if (file != NULL)
    fclose (file);
  errno = saved_errno;
  return -1;
}

