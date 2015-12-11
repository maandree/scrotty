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
#define _GNU_SOURCE /* For getopt_long. */
#include "common.h"
#include "kern.h"
#include "info.h"
#include "pnm.h"
#include "png.h"
#include "pattern.h"

#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#ifdef USE_GETTEXT
# include <locale.h>
#endif



/**
 * X-macro that lists all environment variables
 * that indicate that the program is running
 * inside a display server.
 */
#define LIST_DISPLAY_VARS  X(DISPLAY) X(MDS_DISPLAY) X(MIR_DISPLAY) X(WAYLAND_DISPLAY) X(PREFERRED_DISPLAY)



/**
 * `argv[0]` from `main`.
 */
const char *execname;

/**
 * If a function fails when it tries to
 * open a file, it will set this variable
 * point to the pathname of that file.
 */
const char *failure_file = NULL;

/**
 * The index of the alternative path-pattern,
 * for the framebuffers, to try.
 */
static int try_alt_fbpath = 0;



/**
 * Create an image of a framebuffer.
 * 
 * @param   fbname    The framebuffer device.
 * @param   imgname   The pathname of the output image.
 * @param   width     The width of the image.
 * @param   height    The height of the image.
 * @param   raw       Save in PNM?
 * @param   data      Additional data for `convert_fb_to_pnm`
 *                    and `convert_fb_to_png`.
 * @return            Zero on success, -1 on error.
 */
static int
save (const char *fbpath, const char *imgpath, long width,
      long height, int raw, void *restrict data)
{
  int imgfd = -1, fbfd;
  int saved_errno;
  
  /* Open output file. */
  imgfd = open (imgpath, O_WRONLY | O_CREAT | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (imgfd == -1)
    FILE_FAILURE (imgpath);
  
  /* Open the framebuffer device for reading. */
  fbfd = open (fbpath, O_RDONLY);
  if (fbfd == -1)
    FILE_FAILURE (fbpath);
  
  /* Save image. */
  if ((raw ? save_pnm : save_png) (fbfd, width, height, imgfd, data) < 0)
    goto fail;
  
  close (fbfd);
  close (imgfd);
  return 0;
  
 fail:
  saved_errno = errno;
  if (fbfd >= 0)
    close (fbfd);
  if (imgfd >= 0)
    close (imgfd);
  errno = saved_errno;
  return -1;
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
  char **args = NULL;
  char *arg;
  size_t i, arg_count = 1;
  pid_t pid;
  int status, saved_errno;
  
  /* Count arguments. */
  for (i = 0; flatten_args[i]; i++)
    if ((unsigned char)(flatten_args[i]) == 255)
      arg_count++;
  
  /* Allocate argument array. */
  args = malloc ((arg_count + 1) * sizeof (char*));
  if (args == NULL)
    goto fail;
  
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
    goto fail;

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
    goto fail;
  
  /* Return successfully if and only if `the child` did. */
  free (args);
  return status == 0 ? 0 : -1;
  
 fail:
  saved_errno = errno;
  free (args);
  errno = saved_errno;
  return -1;
}


/**
 * Take a screenshot of a framebuffer.
 * 
 * @param   fbno         The number of the framebuffer.
 * @param   raw          Save in PNM rather than in PNG?.
 * @param   filepattern  The pattern for the filename, `NULL` for default.
 * @param   execpattern  The pattern for the command to run to
 *                       process the image, `NULL` for none.
 * @return               Zero on success, -1 on error, 1 if the framebuffer does not exist.
 */
static int
save_fb (int fbno, int raw, const char *filepattern, const char *execpattern)
{
  char imgpath_[sizeof ("fb.xyz.") + 2 * 3 * sizeof (int)];
  char *imgpath = imgpath_;
  char *fbpath; /* Statically allocate string is returned. */
  char *execargs = NULL;
  long width, height;
  void *data = NULL;
  int i, rc = 0, saved_errno = 0;
  
  /* Get pathname for framebuffer, and stop if we have read all existing ones. */
  fbpath = get_fbpath (try_alt_fbpath, fbno);
  if (access (fbpath, F_OK))
    return 1;
  
  /* Get the size of the framebuffer. */
  if (measure (fbno, fbpath, &width, &height, &data) < 0)
    goto fail;
  
  /* Get output pathname. */
  if (filepattern == NULL)
    {
      sprintf (imgpath, "fb%i.%s", fbno, (raw ? "pnm" : "png"));
      for (i = 2; access (imgpath, F_OK) == 0; i++)
	sprintf (imgpath, "fb%i.%s.%i", fbno, (raw ? "pnm" : "png"), i);
    }
  else
    {
      imgpath = evaluate (filepattern, fbno, width, height, NULL);
      if (imgpath == NULL)
	goto fail;
    }
  
  /* Take a screenshot of the current framebuffer. */
  if (save (fbpath, imgpath, width, height, raw, data) < 0)
    goto fail;
  fprintf (stderr, _("Saved framebuffer %i to %s.\n"), fbno, imgpath);
  
  /* Should we run a command over the image? */
  if (execpattern == NULL)
    goto done;
  
  /* Get execute arguments. */
  execargs = evaluate (execpattern, fbno, width, height, imgpath);
  if (execargs == NULL)
    goto fail;
  
  /* Run command over image. */
  if (exec_image (execargs) < 0)
    goto fail;
  
  goto done;
  
 fail:
  saved_errno = errno;
  rc = -1;
 done:
  free (execargs);
  if (imgpath != imgpath_)
    free (imgpath);
  return errno = saved_errno, rc;
}


/**
 * Take a screenshot of all framebuffers.
 * 
 * @param   raw          Save in PNM rather than in PNG?.
 * @param   filepattern  The pattern for the filename, `NULL` for default.
 * @param   execpattern  The pattern for the command to run to
 *                       process thes image, `NULL` for none.
 * @return               Zero on success, -1 on error, 1 if no framebuffer exists.
 */
static
int save_fbs (int raw, const char *filepattern, const char *exec)
{
  int r, fbno, found = 0;
  
 retry:
  /* Take a screenshot of each framebuffer. */
  for (fbno = 0;; fbno++)
    {
      r = save_fb (fbno, raw, filepattern, exec);
      if (r < 0)
	goto fail;
      else if (r == 0)
	found = 1;
      else if (fbno > 0)
	break;
      else
	continue; /* Perhaps framebuffer 1 is the first. */
    }
  
  /* Did not find any framebuffer? */
  if (found == 0)
    {
      if (try_alt_fbpath++ < alt_fbpath_limit)
	goto retry;
      return 1;
    }
  
  return 0;
 fail:
  return -1;
}


/**
 * Figure out whether the user is in a display server.
 * We will print a warning in `main` if so.
 */
static int
have_display (void)
{
  char *env;
#define X(VAR)  env = getenv(#VAR); if (env && *env) return 1;
  LIST_DISPLAY_VARS
  return 0;
}


/**
 * Take a screenshot of all framebuffers.
 * 
 * @param   argc  The number of elements in `argv`.
 * @param   argv  Command line arguments, run with `--help` for more information.
 * @return        Zero on and only on success.
 */
int
main (int argc, char *argv[])
{
#define EXIT_USAGE(MSG)  \
  return fprintf (stderr, _("%s: %s. Type '%s --help' for help.\n"), execname, MSG, execname), 2
#define USAGE_ASSERT(ASSERTION, MSG)  \
  do { if (!(ASSERTION))  EXIT_USAGE (MSG); } while (0)
  
  int r, raw = 0;
  char *exec = NULL;
  char *filepattern = NULL;
  struct option long_options[] =
    {
      {"help",      no_argument,       NULL, 'h'},
      {"version",   no_argument,       NULL, 'v'},
      {"copyright", no_argument,       NULL, 'c'},
      {"raw",       no_argument,       NULL, 'r'},
      {"exec",      required_argument, NULL, 'e'},
      {NULL,        0,                 NULL,  0 }
    };
  
  /* Set up for internationalisation. */
#if defined(USE_GETTEXT) && defined(PACKAGE) && defined(LOCALEDIR)
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif
  
  /* Parse command line. */
  execname = argc ? *argv : "scrotty";
  for (;;)
    {
      r = getopt_long (argc, argv, "hvcre:", long_options, NULL);
      if      (r == -1)   break;
      else if (r == 'h')  return -(print_help ());
      else if (r == 'v')  return -(print_version ());
      else if (r == 'c')  return -(print_copyright ());
      else if (r == 'r')  raw = 1;
      else if (r == 'e')
	{
	  USAGE_ASSERT (exec == NULL, _("--exec is used twice."));
	  exec = optarg;
	}
      else if (r == '?')
	EXIT_USAGE (_("Invalid input."));
      else
	abort ();
    }
  while (optind < argc)
    {
      USAGE_ASSERT (filepattern == NULL, _("FILENAME-PATTERN is used twice."));
      filepattern = argv[optind++];
    }
  
  /* Take a screenshot of each framebuffer. */
  r = save_fbs (raw, filepattern, exec);
  if (r < 0)
    goto fail;
  if (r > 0)
    goto no_fb;
  
  /* Warn about being inside a display server. */
  if (have_display ())
    fprintf (stderr, _("%s: It looks like you are inside a display server. "
		       "If this is correct, what you see is probably not "
		       "what you get.\n"), execname);
  
  return 0;
  
 fail:
  if (failure_file == NULL)
    perror (execname);
  else
    fprintf (stderr, _("%s: %s: %s\n"),
	     execname, strerror (errno), failure_file);
  return 1;
  
 no_fb:
  print_not_found_help ();
  return 1;
}

