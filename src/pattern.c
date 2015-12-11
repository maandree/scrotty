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
#include "pattern.h"



/**
 * Duplicate all '%':s in a buffer until the first occurrence of a zero byte.
 * 
 * @param   buf  The buffer.
 * @param   n    The size of the buffer.
 * @return       -1 if the buffer is too small, otherwise.
 *               the new position of the first zero byte.
 */
static ssize_t
duplicate_percents (char *restrict buf, size_t n)
{
  size_t p = 0, pi, pc = 0, i;
  
  /* Count number of '%':s. */
  for (i = 0; buf[i]; i++)
    if (buf[i] == '%')
      pc++;
  
  /* Check whether the string will overflow. */
  if (i + pc > n)
    return -1;
  
  /* Duplicate all '%':s. */
  for (pi = 0; pi < pc; pi++)
    {
      p = (size_t)(strchr (buf + p, '%') - buf);
      memmove (buf + p + 1, buf + p, (i - (p - pi)) * sizeof (char));
      p += 2;
    }
  
  return (ssize_t)(i + pi);
}


/**
 * Parse and evaluate a --exec argument or filename pattern.
 * 
 * If `path != NULL` than all non-escaped spaces in
 * `pattern` will be stored as 255-bytes in `buf`.
 * 
 * @param   buf      The output buffer.
 * @param   n        The size of `buf`.
 * @param   pattern  The pattern to evaluate.
 * @param   fbno     The index of the framebuffer.
 * @param   width    The width of the image/framebuffer.
 * @param   height   The height of the image/framebuffer.
 * @param   path     The filename of the saved image, `NULL`
 *                   during the evaluation of the filename pattern.
 * @return           Zero on success, -1 on error.
 */
static int
try_evaluate (char *restrict buf, size_t n, const char *restrict pattern,
	      int fbno, long width, long height, const char *restrict path)
{
#define P(format, value)  r = snprintf (buf + i, n - i, format "%zn", value, &j)
  
  size_t i = 0;
  ssize_t j = 0;
  int percent = 0, backslash = 0, dollar = 0, r;
  char c;
  char *fmt = NULL;
  time_t t;
  struct tm *tm;
  int saved_errno;
  
  /* Expand '$' and '\'. */
  while ((i < n) && ((c = *pattern++)))
    if (dollar)
      {
	dollar = 0;
	if (path == NULL)
	  if ((c == 'f') || (c == 'n'))
	    continue;
	if      (c == 'i')  P ("%i", fbno);
	else if (c == 'f')  P ("%s", path);
	else if (c == 'n')  P ("%s", strrchr (path, '/') ? (strrchr (path, '/') + 1) : path);
	else if (c == 'p')  P ("%ju", (uintmax_t)width * (uintmax_t)height);
	else if (c == 'w')  P ("%li", width);
	else if (c == 'h')  P ("%li", height);
	else if (c == '$')  r = 0, j = 1, buf[i] = '$';
	else if ((r < 0) || (j <= 0))
	  return -1;
	else if ((c == 'f') || (c == 'n'))
	  if (j = duplicate_percents (buf + i, n - i), j < 0)
	    goto enametoolong;
	i += (size_t)j;
      }
    else if (backslash)  buf[i++] = (c == 'n' ? '\n' : c), backslash = 0;
    else if (percent)    buf[i++] = c, percent = 0;
    else if (c == '%')   buf[i++] = c, percent = 1;
    else if (c == '\\')  backslash = 1;
    else if (c == '$')   dollar = 1;
    else if (c == ' ')   buf[i++] = path == NULL ? ' ' : (char)255; /* 255 is not valid in UTF-8. */
    else                 buf[i++] = c;
  if (i >= n)
    goto enametoolong;
  buf[i] = '\0';
  
  /* Check whether there are any '%' to expand. */
  if (strchr (buf, '%') == NULL)
    return 0;
  
  /* Copy the buffer so we can reuse the buffer and use its old content for the format. */
  fmt = strdup (buf);
  if (fmt == NULL)
    goto fail;
  
  /* Expand '%'. */
  t = time (NULL);
  tm = localtime (&t);
  if (tm == NULL)
    goto fail;
  
#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
  if (strftime (buf, n, fmt, tm) == 0)
    goto enametoolong; /* No errors are defined for `strftime`. What else can we do? */
  
  free (fmt);
  return 0;
  
 enametoolong:
  errno = ENAMETOOLONG;
 fail:
  saved_errno = errno;
  free (fmt);
  errno = saved_errno;
  return -1;
}


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
char*
evaluate (const char *restrict pattern, int fbno, long width,
	  long height, const char *restrict path)
{
  char *buffer = NULL;
  size_t size = 32;
  void *new;
  int saved_errno;
  
 retry:
  new = realloc (buffer, size * sizeof(char));
  if (new == NULL)
    goto fail;
  buffer = new;
  
  if (try_evaluate (buffer, size, pattern, fbno, width, height, path) < 0)
    {
      if (errno == ENAMETOOLONG)
	goto retry;
      size <<= 1;
      goto fail;
    }
  
  return buffer;
  
 fail:
  saved_errno = errno;
  free(buffer);
  errno = saved_errno;
  return NULL;
}

