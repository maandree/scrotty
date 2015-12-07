/**
 * scrotty — Screenshot program for Linux's TTY
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
#ifndef PROGRAM_NAME
# define PROGRAM_NAME  "scrotty"
#endif
#ifndef PROGRAM_VERSION
# define PROGRAM_VERSION  "1.1"
#endif


#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <alloca.h>
#include <time.h>
#ifdef USE_GETTEXT
# include <libintl.h>
#endif


#ifndef PATH_MAX
# define PATH_MAX  4096
#endif
#ifndef DEVDIR
# define DEVDIR  "/dev"
#endif
#ifndef SYSDIR
# define SYSDIR  "/sys"
#endif



#ifdef USE_GETTEXT
# define _(STR)  gettext (STR)
#else
# define _(STR)  STR
#endif



/**
 * Stores a filename to `failure_file` and goes to the label `fail`.
 * 
 * @param  PATH:char *  The file that could not be used.
 *                      Must be accessible by `main`.
 */
#define FILE_FAILURE(PATH)	\
  do				\
    {				\
      failure_file = PATH;	\
      goto fail;		\
    }				\
  while (0)



#define LIST_0_9(P)  P"0\n", P"1\n", P"2\n", P"3\n", P"4\n", P"5\n", P"6\n", P"7\n", P"8\n", P"9\n"

/**
 * [0, 255]-integer-to-text convertion lookup table for faster conversion from
 * raw framebuffer data to the PNM format. The values have a whitespace at the
 * end for even faster conversion.
 * Lines should not be longer than 70 (although most programs will probably
 * work even if there are longer lines), therefore the selected whitespace
 * is LF (new line).
 */
static const char* inttable[] =
  {
    LIST_0_9(""),  LIST_0_9("1"), LIST_0_9("2"), LIST_0_9("3"), LIST_0_9("4"),
    LIST_0_9("5"), LIST_0_9("6"), LIST_0_9("7"), LIST_0_9("8"), LIST_0_9("9"),
    
    LIST_0_9("10"), LIST_0_9("11"), LIST_0_9("12"), LIST_0_9("13"), LIST_0_9("14"),
    LIST_0_9("15"), LIST_0_9("16"), LIST_0_9("17"), LIST_0_9("18"), LIST_0_9("19"),
    
    LIST_0_9("20"), LIST_0_9("21"), LIST_0_9("22"), LIST_0_9("23"), LIST_0_9("24"),
    "250\n", "251\n", "252\n", "253\n", "254\n", "255\n"
  };


/**
 * `argv[0]` from `main`.
 */
static const char *execname;

/**
 * Arguments for `convert`,
 * the output file should be printed into `convert_args[2]`.
 * `NULL` is `convert` shall not be used.
 */
static char **convert_args = NULL;

/**
 * If a function fails when it tries to
 * open a file, it will set this variable
 * point to the pathname of that file.
 */
static const char *failure_file = NULL;


/**
 * Create an PNM-file that is sent to `convert` for convertion
 * to a compressed format, or directly to a file.
 * 
 * @param   fbname  The framebuffer device.
 * @param   width   The width of the image.
 * @param   height  The height of the image.
 * @param   fd      The file descriptor connected to `convert`'s stdin
 * @return          Zero on success, -1 on error.
 */
static int
save_pnm (const char *fbpath, long width, long height, int fd)
{
  char buf[PATH_MAX];
  FILE *file = NULL;
  int fbfd = 1;
  int r, g, b;
  ssize_t got, off;
  uint32_t *pixel;
  int saved_errno;
  
  /* Open the framebuffer device for reading. */
  fbfd = open (fbpath, O_RDONLY);
  if (fbfd == -1)
    goto fail;
  
  /* Create a FILE*, for writing, for the image file. */
  file = fdopen (fd, "w");
  if (file == NULL)
    goto fail;
  
  /* The PNM image should begin with `P3\n%{width} %{height}\n%{colour max=255}\n`.
     ('\n' and ' ' can be exchanged at will.) */
  if (fprintf (file, "P3\n%li %li\n255\n", width, height) < 0)
    goto fail;
  
  /* Convert raw framebuffer data into an PNM image. */
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
      for (off = 0; off < got; off += 4)
	{
	  /* A pixel in the framebuffer is formatted as `%{blue}%{green}%{red}%{x}`
	     in big-endian binary, or `%{x}%{red}%{green}%{blue}` in little-endian binary. */
	  pixel = (uint32_t *)(buf + off);
	  r = (*pixel >> 16) & 255;
	  g = (*pixel >> 8) & 255;
	  b = (*pixel >> 0) & 255;
	  /* A pixel in the PNM image is formatted as `%{red} %{green} %{blue} ` in text. */
	  if (fprintf (file, "%s%s%s", inttable[r], inttable[g], inttable[b]) < 0)
	    goto fail;
	}
      
      /* If we read a whole number of pixels, reset the buffer, otherwise,
         move the unconverted bytes to the beginning of the buffer. */
      if (off != got)
	{
	  off -= 4;
	  memcpy (buf, buf + off, (size_t)(got - off) * sizeof (char));
	  off = got - off;
	}
      else
	off = 0;
    }
  
  /* Close files and return successfully. */
  fflush (file);
  fclose (file);
  close (fbfd);
  return 0;
  
 fail:
  saved_errno = errno;
  if (file != NULL)
    fclose (file);
  if (fbfd >= 0)
    close (fbfd);
  errno = saved_errno;
  return -1;
}


/**
 * Create an image of a framebuffer.
 * 
 * @param   fbname    The framebuffer device.
 * @param   imgname   The pathname of the output image.
 * @param   width     The width of the image.
 * @param   height    The height of the image.
 * @return            Zero on success, -1 on error.
 */
static int
save (const char *fbpath, const char *imgpath, long width, long height)
{
  int pipe_rw[2] = { -1, -1 };
  pid_t pid;
  int status;
  int fd = -1;
  int saved_errno;
  
  if (convert_args == NULL)
    goto no_convert;
  
  /* Create a pipe that for sending data into the `convert` program. */
  if (pipe (pipe_rw) < 0)
    goto fail;
  
  /* Fork the process, the child will exec. `convert`. */
  pid = fork ();
  if (pid == -1)
    goto fail;
  
  /* Child process: */
  if (pid == 0)
    {
      /* Close the write-end of the pipe. */
      close (pipe_rw[1]);
      /* Turn the read-end of the into stdin. */
      if (pipe_rw[0] != STDIN_FILENO)
	{
	  close (STDIN_FILENO);
	  if (dup2 (pipe_rw[0], STDIN_FILENO) == -1)
	    goto child_fail;
	  close (pipe_rw[0]);
	}
      /* Exec. `convert` to convert the PNM-image we create to a compressed image. */
      sprintf (convert_args[2], "%s", imgpath);
      execvp ("convert", convert_args);
      
    child_fail:
      perror(execname);
      _exit(1);
    }
  
  /* Parent process: */
  
  /* Close the read-end of the pipe. */
  close (pipe_rw[0]), pipe_rw[0] = -1;
  
  /* Create a PNM-image of the framebuffer. */
  if (save_pnm (fbpath, width, height, pipe_rw[1]) < 0)
    goto fail;
  
  /* Close the write-end of the pipe. */
  close (pipe_rw[1]), pipe_rw[1] = -1;
  
  /* Wait for `convert` to exit. */
  if (waitpid (pid, &status, 0) < 0)
    goto fail;
  
  /* Return successfully if and only if `convert` did. */
  return status == 0 ? 0 : -1;
  
  
  /* `convert` shall not be used: */
  
 no_convert:
  /* Open output file. */
  if (fd = open (imgpath, O_WRONLY | O_CREAT | O_TRUNC,
		 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH), fd == -1)
    FILE_FAILURE (imgpath);
  
  /* Save image. */
  if (save_pnm (fbpath, width, height, fd) < 0)
    goto fail;
  
  close (fd);
  return 0;
  
 fail:
  saved_errno = errno;
  if (pipe_rw[0] >= 0)
    close (pipe_rw[0]);
  if (pipe_rw[1] >= 0)
    close (pipe_rw[1]);
  if (fd >= 0)
    close (fd);
  errno = saved_errno;
  return -1;
}


/**
 * Get the dimensions of the framebuffer.
 * 
 * @param   fbno    The number of the framebuffer.
 * @param   width   Output parameter for the width of the image.
 * @param   height  Output parameter for the height of the image.
 * @return          Zero on success, -1 on error.
 */
static int
measure (int fbno, long *width, long *height)
{
  static char buf[PATH_MAX];
  char *delim;
  int sizefd = -1;
  ssize_t got;
  int saved_errno;
  
  /* Open the file with the framebuffer's dimensions. */
  sprintf (buf, SYSDIR "/class/graphics/fb%i/virtual_size", fbno);
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
}


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
 * @param   path     The filename of the saved image, `NULL` during the evaluation of the filename pattern.
 * @return           Zero on success, -1 on error.
 */
static int evaluate (char *restrict buf, size_t n, const char *restrict pattern,
		     int fbno, long width, long height, const char *restrict path)
{
#define P(format, value)  r = snprintf (buf + i, n - i, format "%zn", value, &j)
  
  size_t i = 0;
  ssize_t j = 0;
  int percent = 0, backslash = 0, dollar = 0, r;
  char c;
  char* fmt;
  time_t t;
  struct tm tm;
  
  /* Expand '$' and '\'. */
  while ((c = *pattern++))
    {
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
	  else
	    continue;
	  if ((r < 0) || (j <= 0))
	    return -1;
	  if ((c == 'f') || (c == 'n'))
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
    }
  buf[i] = '\0';
  
  /* Check whether there are any '%' to expand. */
  if (strchr (buf, '%') == NULL)
    return 0;
  
  /* Copy the buffer so we can reuse the buffer and use its old content for the format. */
  fmt = alloca ((strlen (buf) + 1) * sizeof (char));
  memcpy (fmt, buf, (strlen (buf) + 1) * sizeof (char));
  
  /* Expand '%'. */
  t = time (NULL);
  localtime_r (&t, &tm);
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
  if (strftime (buf, n, fmt, &tm) == 0)
    goto enametoolong; /* No errors are defined for `strftime`. */
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
  
  return 0;
  
 enametoolong:
  return errno = ENAMETOOLONG, -1;
  
#undef P
}


/**
 * Run a command for an image
 * 
 * @param   flatten_args  The arguments to run, 255 delimits the arguments
 * @return                Zero on success -1 on error
 */
static int
exec_image (char *flatten_args)
{
  char **args;
  char *arg;
  size_t i, arg_count = 1;
  pid_t pid;
  int status;
  
  /* Count arguments. */
  for (i = 0; flatten_args[i]; i++)
    if ((unsigned char)(flatten_args[i]) == 255)
      arg_count++;
  
  /* Allocate argument array. */
  args = alloca ((arg_count + 1) * sizeof (char*));
  
  /* Unflatten argument array. */
  for (arg = flatten_args, i = 0;;)
    {
      args[i++] = arg;
      arg = strchr (arg, 255);
      if (arg == NULL)
	break;
      *arg++ = '\0';
    }
  args[i] = NULL;
  
  /* Fork process. */
  pid = fork ();
  if (pid == -1)
    return -1;

  /* Child process: */
  if (pid == 0)
    {
      execvp (*args, args);
      perror (execname);
      _exit (1);
    }
  
  /* Parent process: */
  
  /* Wait for child to exit. */
  if (waitpid (pid, &status, 0) < 0)
    return -1;
  
  /* Return successfully if and only if `the child` did. */
  return status == 0 ? 0 : -1;
}


/**
 * Take a screenshot of a framebuffer.
 * 
 * @param   fbno         The number of the framebuffer.
 * @param   raw          Save in PNM rather than in PNG?.
 * @param   filepattern  The pattern for the filename, `NULL` for default.
 * @param   execpattern  The pattern for the command to run to
 *                       process the image, `NULL` for default.
 * @return               Zero on success, -1 on error, 1 if the framebuffer does not exist.
 */
static int
save_fb (int fbno, int raw, const char *filepattern, const char *execpattern)
{
  static char imgpath[PATH_MAX];
  char fbpath[PATH_MAX];
  char *execargs = NULL;
  void *new;
  long width, height;
  size_t size = PATH_MAX;
  int i, saved_errno;
  
  /* Get pathname for framebuffer, and stop if we have read all existing ones. */
  sprintf (fbpath, DEVDIR "/fb%i", fbno);
  if (access (fbpath, F_OK))
    return 1;
  
  /* Get the size of the framebuffer. */
  if (measure (fbno, &width, &height) < 0)
    goto fail;

  /* Get output pathname. */
  if (filepattern == NULL)
    {
      sprintf (imgpath, "fb%i.%s", fbno, (raw ? "pnm" : "png"));
      if (access (imgpath, F_OK) == 0)
	for (i = 2;; i++)
	  {
	    sprintf (imgpath, "fb%i.%s.%i", fbno, (raw ? "pnm" : "png"), i);
	    if (access (imgpath, F_OK))
	      break;
	  }
    }
  else
    if (evaluate (imgpath, (size_t)PATH_MAX, filepattern, fbno, width, height, NULL) < 0)
      return -1;
  
  /* Take a screenshot of the current framebuffer. */
  if (save (fbpath, imgpath, width, height) < 0)
    goto fail;
  fprintf (stderr, _("Saved framebuffer %i to %s,\n"), fbno, imgpath);
  
  /* Should we run a command over the image? */
  if (execpattern == NULL)
    return 0;
  
  /* Get execute arguments. */
 retry:
  new = realloc (execargs, size * sizeof (char));
  if (execargs == NULL)
    goto fail;
  execargs = new;
  if (evaluate (execargs, size, execpattern, fbno, width, height, imgpath) < 0)
    {
      if ((errno != ENAMETOOLONG) || ((size >> 8) <= PATH_MAX))
	goto fail;
      size <<= 1;
      goto retry;
    }
  
  /* Run command over image. */
  if (exec_image (execargs) < 0)
    goto fail;
  free (execargs);
  return 0;
  
 fail:
  saved_errno = errno;
  free (execargs);
  errno = saved_errno;
  return -1;
}


/**
 * Print usage information.
 * 
 * @return  Zero on success, -1 on error.
 */
static int
print_help(void)
{
  return printf (_("SYNOPSIS\n"
		   "\t%s [options...] [filename-pattern] [-- options-for-convert...]\n"
		   "\n"
		   "OPTIONS\n"
		   "\t--help         Print usage information.\n"
		   "\t--version      Print program name and version.\n"
		   "\t--copyright    Print copyright information.\n"
		   "\t--raw          Save in PNM rather than in PNG.\n"
		   "\t--exec CMD     Command to run for each saved image.\n"
		   "\n"
		   "SPECIAL STRINGS\n"
		   "\tBoth the --exec and filename-pattern parameters can take format specifiers\n"
		   "\tthat are expanded by scrotty when encountered. There are two types of format\n"
		   "\tspecifier. Characters preceded by a '%%' are interpreted by strftime(3).\n"
		   "\tSee `man strftime` for examples. These options may be used to refer to the\n"
		   "\tcurrent date and time. The second kind are internal to scrotty and are prefixed\n"
		   "\tby '$' or '\\'. The following specifiers are recognised:\n"
		   "\n"
		   "\t$i  framebuffer index\n"
		   "\t$f  image filename/pathname (ignored when used in filename-pattern)\n"
		   "\t$n  image filename          (ignored when used in filename-pattern)\n"
		   "\t$p  image width multiplied by image height\n"
		   "\t$w  image width\n"
		   "\t$h  image height\n"
		   "\t$$  expands to a literal '$'\n"
		   "\t\\n  expands to a new line\n"
		   "\t\\\\  expands to a literal '\\'\n"
		   "\t\\   expands to a literal ' ' (the string is a backslash followed by a space)\n"
		   "\n"
		   "\tA space that is not prefixed by a backslash in --exec is interpreted as an\n"
		   "\targument delimiter. This is the case even at the beginning and end of the\n"
		   "\tstring and if a space was the previous character in the string.\n"
		   "\n"),
		 execname) < 0 ? -1 : 0;
}


/**
 * Print program name and version.
 * 
 * @return  Zero on success, -1 on error.
 */
static int
print_version (void)
{
  return printf (_("%s\n"
		   "Copyright (C) %s.\n"
		   "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
		   "This is free software: you are free to change and redistribute it.\n"
		   "There is NO WARRANTY, to the extent permitted by law.\n"
		   "\n"
		   "Written by Mattias Andrée.\n"),
		 PROGRAM_NAME " " PROGRAM_VERSION,
		 "2014, 2015 Mattias Andrée") < 0 ? -1 : 0;
}


/**
 * Print copyright information.
 * 
 * @return  Zero on success, -1 on error.
 */
static int
print_copyright (void)
{
  return printf (_("scrotty -- Screenshot program for Linux's TTY\n"
		   "Copyright (C) %s\n"
		   "\n"
		   "This program is free software: you can redistribute it and/or modify\n"
		   "it under the terms of the GNU General Public License as published by\n"
		   "the Free Software Foundation, either version 3 of the License, or\n"
		   "(at your option) any later version.\n"
		   "\n"
		   "This program is distributed in the hope that it will be useful,\n"
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		   "GNU General Public License for more details.\n"
		   "\n"
		   "You should have received a copy of the GNU General Public License\n"
		   "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"),
		 "2014, 2015  Mattias Andrée (maandree@member.fsf.org)"
		 ) < 0 ? -1 : 0;
}


/**
 * Take a screenshot of all framebuffers
 * 
 * @param   argc  The number of elements in `argv`
 * @param   argv  Command line arguments
 * @return        Zero on and only on success
 */
int
main (int argc, char *argv[])
{
#define EXIT_USAGE(MSG)  \
  return fprintf (stderr, _("%s: %s. Type '%s --help' for help.\n"), execname, MSG, execname), 1
  
  int fbno, r, i, dash = argc, exec = -1, help = 0;
  int raw = 0, version = 0, copyright = 0, filepattern = -1;
  static char convert_args_0[] = "convert";
  static char convert_args_1[] = DEVDIR "/stdin";
  static char convert_args_2[PATH_MAX];
  
  execname = argc ? *argv : "scrotty";
  
  /* Set up for internationalisation. */
#if defined(USE_GETTEXT) && defined(PACKAGE) && defined(LOCALEDIR)
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif
  
  /* Parse command line. */
  for (i = 1; i < argc; i++)
    {
      if      (!strcmp (argv[i], "--help"))       help = 1;
      else if (!strcmp (argv[i], "--version"))    version = 1;
      else if (!strcmp (argv[i], "--copyright"))  copyright = 1;
      else if (!strcmp (argv[i], "--raw"))        raw = 1;
      else if (!strcmp (argv[i], "--exec"))       exec = ++i;
      else if (!strcmp (argv[i], "--"))
	{
	  dash = i + 1;
	  break;
	}
      else if ((argv[i][0] == '-') || (filepattern != -1))
	EXIT_USAGE (_("Unrecognised option."));
      else
	filepattern = i;
    }
  
  /* Check that --exec is valid. */
  if (exec == argc)
    EXIT_USAGE (_("--exec has no argument."));
  
  /* Was --help, --version, or --copyright used? */
  if (help)       return -(print_help ());
  if (version)    return -(print_version ());
  if (copyright)  return -(print_copyright ());
  
  /* Create arguments for `convert`. */
  if ((!raw) || (dash < argc))
    {
      convert_args = alloca ((size_t)(4 + (argc - dash)) * sizeof (char*));
      convert_args[0] = convert_args_0;
      convert_args[1] = convert_args_1;
      convert_args[2] = convert_args_2;
      for (i = dash; i < argc; i++)
	convert_args[i - dash + 2] = argv[i];
      convert_args[3 + (argc - dash)] = NULL;
    }
  
  /* Take a screenshot of each framebuffer. */
  for (fbno = 0;; fbno++)
    {
      r = save_fb (fbno, raw,
		   (filepattern < 0 ? NULL : argv[filepattern]),
		   (exec < 0 ? NULL : argv[exec]));
      if (r < 0)
	goto fail;
      if (r > 0)
	break;
    }
  
  return 0;
  
 fail:
  if (failure_file == NULL)
    perror (execname);
  else
    fprintf (stderr, _("%s: %s: %s\n"),
	     execname, strerror (errno), failure_file);
  return 1;
  
#undef EXIT_USAGE
}

