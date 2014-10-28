/**
 * scrotty — Screenshot program for Linux's TTY
 * Copyright © 2014  Mattias Andrée (maandree@member.fsf.org)
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
# define PROGRAM_VERSION  "1.0"
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


#ifndef PATH_MAX
# define PATH_MAX  4096
#endif
#ifndef DEVDIR
# define DEVDIR  "/dev"
#endif
#ifndef SYSDIR
# define SYSDIR  "/sys"
#endif



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
 * `argv[0]` from `main`
 */
static const char* execname;

/**
 * Arguments for `convert`,
 * the output file should be printed into `convert_args[2]`.
 */
static char** convert_args;


/**
 * Create an PNM-file that is sent to `convert` for convertion to a compressed format
 * 
 * @param   fbname  The framebuffer device
 * @param   width   The width of the image
 * @param   height  The height of the image
 * @param   fd      The file descriptor connected to `convert`'s stdin
 * @return          Zero on success, -1 on error
 */
static int save_pnm(const char* fbpath, long width, long height, int fd)
{
  char buf[PATH_MAX];
  FILE* file;
  int saved_errno, fbfd, r, g, b;
  ssize_t got, off;
  
  /* Open the framebuffer device for reading. */
  if (fbfd = open(fbpath, O_RDONLY), fbfd < 0)
    return -1;
  
  /* Create a FILE*, for writing, for the image file. */
  file = fdopen(fd, "w");
  if (file == NULL)
    return saved_errno = errno, close(fbfd), errno = saved_errno, -1;
  
  /* The PNM image should begin with `P3\n%{width} %{height}\n%{colour max=255}\n`.
     ('\n' and ' ' can be exchanged at will.) */
  fprintf(file, "P3\n%li %li\n255\n", width, height);
  
  /* Convert raw framebuffer data into an PNM image. */
  for (off = 0;;)
    {
      /* Read data from the framebuffer, we may have up to 3 bytes buffered. */
      got = read(fbfd, buf + off, sizeof(buf) - (size_t)off * sizeof(char));
      if (got < 0)
	return saved_errno = errno, fclose(file), close(fbfd), errno = saved_errno, -1;
      if (got += off, got == 0)
	break;
      
      /* Convert read pixels. */
      for (off = 0; off < got; off += 4)
	{
	  /* A pixel in the framebuffer is formatted as `%{blue}%{green}%{red}%{x}`
	     in big-endian binary, or `%{x}%{red}%{green}%{blue}` in little-endian binary. */
	  uint32_t* pixel = (uint32_t*)(buf + off);
	  r = (*pixel >> 16) & 255;
	  g = (*pixel >> 8) & 255;
	  b = (*pixel >> 0) & 255;
	  /* A pixel in the PNM image is formatted as `%{red} %{green} %{blue} ` in text. */
	  fprintf(file, "%s%s%s", inttable[r], inttable[g], inttable[b]);
	}
      
      /* If we read a whole number of pixels, reset the buffer, otherwise,
         move the unconverted bytes to the beginning of the buffer. */
      if (off != got)
	{
	  off -= 4;
	  memcpy(buf, buf + off, (size_t)(got - off) * sizeof(char));
	  off = got - off;
	}
      else
	off = 0;
    }
  
  /* Close files and return successfully. */
  fflush(file);
  fclose(file);
  close(fbfd);
  return 0;
}


/**
 * Create an image of a framebuffer
 * 
 * @param   fbname    The framebuffer device
 * @param   imgname   The pathname of the output image
 * @param   width     The width of the image
 * @param   height    The height of the image
 * @return            Zero on success, -1 on error
 */
static int save(const char* fbpath, const char* imgpath, long width, long height)
{
  int pipe_rw[2];
  pid_t pid, reaped;
  int saved_errno, status;
  
  /* Create a pipe that for sending data into the `convert` program. */
  if (pipe(pipe_rw) < 0)
    return -1;
  
  /* Fork the process, the child will exec. `convert`. */
  if (pid = fork(), pid == -1)
    return saved_errno = errno, close(pipe_rw[0]), close(pipe_rw[1]), errno = saved_errno, -1;
  
  /* Child process: */
  if (pid == 0)
    {
      /* Close the write-end of the pipe. */
      close(pipe_rw[1]);
      /* Turn the read-end of the into stdin. */
      if (pipe_rw[0] != STDIN_FILENO)
	{
	  close(STDIN_FILENO);
	  dup2(pipe_rw[0], STDIN_FILENO);
	  close(pipe_rw[0]);
	}
      /* Exec. `convert` to convert the PNM-image we create to a compressed image. */
      sprintf(convert_args[2], "%s", imgpath);
      execvp("convert", convert_args);
      perror(execname);
      exit(1);
    }
  
  /* Parent process: */
  
  /* Close the read-end of the pipe. */
  close(pipe_rw[0]);
  
  /* Create a PNM-image of the framebuffer. */
  if (save_pnm(fbpath, width, height, pipe_rw[1]) < 0)
    return saved_errno = errno, close(pipe_rw[1]), errno = saved_errno, -1;
  
  /* Close the write-end of the pipe. */
  close(pipe_rw[1]);
  
  /* Wait for `convert` to exit. */
  for (reaped = 0; reaped != pid;)
    if (reaped = waitpid(pid, &status, 0), reaped < 0)
      return -1;
  
  /* Return successfully if and only if `convert` did. */
  return status == 0 ? 0 : -1;
}


/**
 * Get the dimensions of the framebuffer
 * 
 * @param   fbno    The number of the framebuffer
 * @param   width   Output parameter for the width of the image
 * @param   height  Output parameter for the height of the image
 * @return          Zero on success, -1 on error
 */
static int measure(int fbno, long* width, long* height)
{
  char buf[PATH_MAX];
  char* delim;
  int sizefd, saved_errno;
  ssize_t got;
  
  /* Open the file with the framebuffer's dimensions. */
  sprintf(buf, SYSDIR "/class/graphics/fb%i/virtual_size", fbno);
  if (sizefd = open(buf, O_RDONLY), sizefd < 0)
    return -1;
  
  /* Get the dimensions of the framebuffer. */
  if (got = read(sizefd, buf, sizeof(buf) / sizeof(char) - 1), got < 0)
    return saved_errno = errno, close(sizefd), errno = saved_errno, -1;
  close(sizefd);
  
  /* The read content is formated as `%{width},%{height}\n`,
     convert it to `%{width}\0%{height}\0` and parse it. */
  buf[got] = '\0';
  delim = strchr(buf, ',');
  *delim++ = '\0';
  *width = atol(buf);
  *height = atol(delim);
  
  return 0;
}


/**
 * Duplicate all '%':s in a buffer until the first occurrence of a zero byte
 * 
 * @param   buf  The buffer
 * @param   n    The size of the buffer
 * @return       -1 if the buffer is too small, otherwise
 *               the new position of the first zero byte
 */
static ssize_t duplicate_percents(char* restrict buf, size_t n)
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
      p = (size_t)(strchr(buf + p, '%') - buf);
      memmove(buf + p + 1, buf + p, (i - (p - pi)) * sizeof(char));
      p += 2;
    }
  
  return (ssize_t)(i + pi);
}


/**
 * Parse and evaluate a --exec argument or filename pattern
 * 
 * If `path != NULL` than all non-escaped spaces in
 * `pattern` will be stored as 255-bytes in `buf`.
 * 
 * @param   buf      The output buffer
 * @param   n        The size of `buf`
 * @param   pattern  The pattern to evaluate
 * @param   fbno     The index of the framebuffer
 * @param   width    The width of the image/framebuffer
 * @param   height   The height of the image/framebuffer
 * @param   path     The filename of the saved image, `NULL` during the evaluation of the filename pattern
 * @return           Zero on success, -1 on error
 */
static int evaluate(char* restrict buf, size_t n, const char* restrict pattern,
		    int fbno, long width, long height, const char* restrict path)
{
#define p(format, value)  r = snprintf(buf + i, n - i, format "%zn", value, &j)
  
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
	  if      (c == 'i')  p("%i", fbno);
	  else if (c == 'f')  p("%s", path);
	  else if (c == 'n')  p("%s", strrchr(path, '/') ? (strrchr(path, '/') + 1) : path);
	  else if (c == 'p')  p("%ju", (uintmax_t)width * (uintmax_t)height);
	  else if (c == 'w')  p("%li", width);
	  else if (c == 'h')  p("%li", height);
	  else if (c == '$')  r = 0, j = 1, buf[i] = '$';
	  else
	    continue;
	  if ((r < 0) || (j <= 0))
	    return -1;
	  if ((c == 'f') || (c == 'n'))
	    if (j = duplicate_percents(buf + i, n - i), j < 0)
	      return errno = ENAMETOOLONG, -1;
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
	return errno = ENAMETOOLONG, -1;
    }
  buf[i] = '\0';
  
  /* Check whether there are any '%' to expand. */
  if (strchr(buf, '%') == NULL)
    return 0;
  
  /* Copy the buffer so we can reuse the buffer and use its old content for the format. */
  fmt = alloca((strlen(buf) + 1) * sizeof(char));
  memcpy(fmt, buf, (strlen(buf) + 1) * sizeof(char));
  
  /* Expand '%'. */
  t = time(NULL);
  localtime_r(&t, &tm);
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wformat-nonliteral"
  if (strftime(buf, n, fmt, &tm) == 0)
# pragma GCC diagnostic pop
    return errno = ENAMETOOLONG, -1;
  
  return 0;
  
#undef p
}


/**
 * Run a command for an image
 * 
 * @param   flatten_args  The arguments to run, 255 delimits the arguments
 * @return                Zero on success -1 on error
 */
static int exec_image(char* flatten_args)
{
  char** args;
  char* arg;
  size_t i, arg_count = 1;
  pid_t pid, reaped;
  int status;
  
  /* Count arguments. */
  for (i = 0; flatten_args[i]; i++)
    if ((unsigned char)(flatten_args[i]) == 255)
      arg_count++;
  
  /* Allocate argument array. */
  args = alloca((arg_count + 1) * sizeof(char*));
  
  /* Unflatten argument array. */
  for (arg = flatten_args, i = 0;;)
    {
      args[i++] = arg;
      arg = strchr(arg, 255);
      if (arg == NULL)
	break;
      *arg++ = '\0';
    }
  args[i] = NULL;

  /* Fork process. */
  pid = fork();
  if (pid == -1)
    return -1;

  /* Child process: */
  if (pid == 0)
    {
      execvp(*args, args);
      perror(execname);
      exit(1);
      return -1;
    }
  
  /* Parent process: */
  
  /* Wait for child to exit. */
  for (reaped = 0; reaped != pid;)
    if (reaped = waitpid(pid, &status, 0), reaped < 0)
      return -1;
  
  /* Return successfully if and only if `the child` did. */
  return status == 0 ? 0 : -1;
}


/**
 * Take a screenshot of a framebuffer
 * 
 * @param   fbno         The number of the framebuffer
 * @param   filepattern  The pattern for the filename, `NULL` for default
 * @return               Zero on success, -1 on error, 1 if the framebuffer does not exist
 */
static int save_fb(int fbno, const char* filepattern, const char* execpattern)
{
  char fbpath[PATH_MAX];
  char imgpath[PATH_MAX];
  char* execargs = NULL;
  char* old;
  long width, height;
  size_t size = PATH_MAX;
  int i, saved_errno;
  
  /* Get pathname for framebuffer, and stop if we have read all existing ones. */
  sprintf(fbpath, DEVDIR "/fb%i", fbno);
  if (access(fbpath, F_OK))
    return 1;
  
  /* Get the size of the framebuffer. */
  if (measure(fbno, &width, &height) < 0)
    return -1;

  /* Get output pathname. */
  if (filepattern == NULL)
    {
      sprintf(imgpath, "fb%i.png", fbno);
      if (access(imgpath, F_OK) == 0)
	for (i = 2;; i++)
	  {
	    sprintf(imgpath, "fb%i.png.%i", fbno, i);
	    if (access(imgpath, F_OK))
	      break;
	  }
    }
  else
    if (evaluate(imgpath, (size_t)PATH_MAX, filepattern, fbno, width, height, NULL) < 0)
      return -1;
  
  /* Take a screenshot of the current framebuffer. */
  if (save(fbpath, imgpath, width, height) < 0)
    return -1;
  fprintf(stderr, "Saved framebuffer %i to %s\n", fbno, imgpath);
  
  /* Should we run a command over the image? */
  if (execpattern == NULL)
    return 0;
  
  /* Get execute arguments. */
 retry:
  execargs = realloc(old = execargs, size * sizeof(char));
  if (execargs == NULL)
    return saved_errno = errno, free(old), errno = saved_errno, -1;
  if (evaluate(execargs, size, execpattern, fbno, width, height, imgpath) < 0)
    {
      if ((errno != ENAMETOOLONG) || ((size >> 8) <= PATH_MAX))
	return -1;
      size <<= 1;
      goto retry;
    }
  
  /* Run command over image. */
  i = exec_image(execargs);
  saved_errno = errno;
  free(execargs);
  return errno = saved_errno, i;
}


#define p(...)  if (printf(__VA_ARGS__) < 0)  return -1;


/**
 * Print usage information
 * 
 * @return  Zero on success, -1 on error
 */
static int print_help(void)
{
  p("SYNOPSIS\n");
  p("\t%s [options...] [filename-pattern] [-- options-for-convert...]\n", execname);
  p("\n");
  p("OPTIONS\n");
  p("\t--help         Print usage information.\n");
  p("\t--version      Print program name and version.\n");
  p("\t--copyright    Print copyright information.\n");
  p("\t--exec CMD     Command to run for each saved image.\n");
  p("\n");
  p("SPECIAL STRINGS\n");
  p("\tBoth the --exec and filename-pattern parameters can take format specifiers\n");
  p("\tthat are expanded by scrotty when encountered. There are two types of format\n");
  p("\tspecifier. Characters preceded by a '%%' are interpretted by strftime(2).\n");
  p("\tSee `man strftime` for examples. These options may be used to refer to the\n");
  p("\tcurrent date and time. The second kind are internal to scrotty and are prefixed\n");
  p("\tby '$' or '\\'. The following specifiers are recognised:\n");
  p("\n");
  p("\t\n");
  p("\t$i  framebuffer index\n");
  p("\t$f  image filename/pathname (ignored when used in filename-pattern)\n");
  p("\t$n  image filename          (ignored when used in filename-pattern)\n");
  p("\t$p  image width multiplied by image height\n");
  p("\t$w  image width\n");
  p("\t$h  image height\n");
  p("\t$$  expands to a literal '$'\n");
  p("\t\\n  expands to a new line\n");
  p("\t\\\\  expands to a literal '\\'\n");
  p("\t\\   expands to a literal ' ' (the string is a backslash followed by a space)\n");
  p("\n");
  p("\tA space that is not prefixed by a backslash in --exec is interpreted as an\n");
  p("\targument delimiter. This is the case even at the beginning and end of the\n");
  p("\tstring and if a space was the previous character in the string.\n");
  p("\n");
  return 0;
}


/**
 * Print program name and version
 * 
 * @return  Zero on success, -1 on error
 */
static int print_version(void)
{
  p("%s %s\n", PROGRAM_NAME, PROGRAM_VERSION);
  return 0;
}


/**
 * Print copyright information
 * 
 * @return  Zero on success, -1 on error
 */
static int print_copyright(void)
{
  p("scrotty -- Screenshot program for Linux's TTY\n");
  p("Copyright (C) 2014  Mattias Andrée (maandree@member.fsf.org)\n");
  p("\n");
  p("This program is free software: you can redistribute it and/or modify\n");
  p("it under the terms of the GNU General Public License as published by\n");
  p("the Free Software Foundation, either version 3 of the License, or\n");
  p("(at your option) any later version.\n");
  p("\n");
  p("This program is distributed in the hope that it will be useful,\n");
  p("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  p("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
  p("GNU General Public License for more details.\n");
  p("\n");
  p("You should have received a copy of the GNU General Public License\n");
  p("along with this program.  If not, see <http://www.gnu.org/licenses/>.\n");
  return 0;
}


#undef p


/**
 * Take a screenshot of all framebuffers
 * 
 * @param   argc  The number of elements in `argv`
 * @param   argv  Command line arguments
 * @return        Zero on and only on success
 */
int main(int argc, char* argv[])
{
  int fbno, r, i, dash = argc, exec = -1, help = 0, version = 0, copyright = 0, filepattern = -1;
  static char convert_args_0[] = "convert";
  static char convert_args_1[] = DEVDIR "/stdin";
  static char convert_args_2[PATH_MAX];
  
  execname = *argv;
  
  /* Parse command line. */
  for (i = 1; i < argc; i++)
    {
      if      (!strcmp(argv[i], "--help"))       help = 1;
      else if (!strcmp(argv[i], "--version"))    version = 1;
      else if (!strcmp(argv[i], "--copyright"))  copyright = 1;
      else if (!strcmp(argv[i], "--exec"))       exec = ++i; /* TODO use this */
      else if (!strcmp(argv[i], "--"))
	{
	  dash = i + 1;
	  break;
	}
      else if ((argv[i][0] == '-') || (filepattern != -1))
	return fprintf(stderr, "Unrecognised option. Type `%s --help` for help.\n", execname), 1;
      else
	filepattern = i;
    }
  
  /* Check that --exec is valid. */
  if (exec == argc)
    return fprintf(stderr, "--exec has no argument. Type `%s --help` for help.\n", execname), 1;
  
  /* Was --help, --version or --copyright used? */
  if (help)       return -(print_help());
  if (version)    return -(print_version());
  if (copyright)  return -(print_copyright());
  
  /* Create arguments for `convert`. */
  convert_args = alloca((size_t)(4 + (argc - dash)) * sizeof(char*));
  convert_args[0] = convert_args_0;
  convert_args[1] = convert_args_1;
  convert_args[2] = convert_args_2;
  for (i = dash; i < argc; i++)
    convert_args[i - dash + 2] = argv[i];
  convert_args[3 + (argc - dash)] = NULL;
  
  /* Take a screenshot of each framebuffer. */
  for (fbno = 0;; fbno++)
    {
      r = save_fb(fbno, filepattern < 0 ? NULL : argv[filepattern], exec < 0 ? NULL : argv[exec]);
      if (r < 0)  return perror(execname), 1;
      if (r > 0)  break;
    }
  
  return 0;
}

