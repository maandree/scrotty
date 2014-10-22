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
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>


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
 * Create an PNM-file that is sent to `convert` for convertion to a compressed format
 * 
 * @param   fbname  The framebuffer device
 * @param   fbno    The number of the framebuffer
 * @param   fd      The file descriptor connected to `convert`'s stdin
 * @return          Zero on success, -1 on error
 */
static int save_pnm(const char* fbpath, int fbno, int fd)
{
  char buf[PATH_MAX];
  FILE* file;
  int saved_errno, sizefd, fbfd, r, g, b;
  ssize_t got, off;
  
  /* Open the file with the framebuffer's dimensions. */
  sprintf(buf, SYSDIR "/class/graphics/fb%i/virtual_size", fbno);
  if (sizefd = open(buf, O_RDONLY), sizefd < 0)
    return -1;
  
  /* Get the dimensions of the framebuffer. */
  if (got = read(sizefd, buf, sizeof(buf) / sizeof(char) - 1), got < 0)
    return saved_errno = errno, close(sizefd), errno = saved_errno, -1;
  close(sizefd);
  /* The read content is formated as `%{1},%{2}\n`, we want it do be `%{1} %{2}\n\0`. */
  buf[got] = '\0';
  *strchr(buf, ',') = ' ';
  
  /* Open the framebuffer device for reading. */
  if (fbfd = open(fbpath, O_RDONLY), fbfd < 0)
    return -1;

  /* Create a FILE*, for writing, for the image file. */
  file = fdopen(fd, "w");
  if (file == NULL)
    return saved_errno = errno, close(fbfd), errno = saved_errno, -1;
  
  /* The PNM image should begin with `P3\n%{width} %{height}\n%{colour max=255}\n`.
     ('\n' and ' ' can be exchanged at will.) */
  fprintf(file, "P3\n%s255\n", buf);
  
  /* Convert raw framebuffer data into an PNM image. */
  for (off = 0;;)
    {
      /* Read data from the framebuffer, we may have up to 3 bytes buffered. */
      got = read(fbfd, buf + off, sizeof(buf) - off * sizeof(char));
      if (got < 0)
	return saved_errno = errno, fclose(file), close(fbfd), errno = saved_errno, -1;
      if (got += off, got == 0)
	break;
      
      /* Convert read pixels. */
      for (off = 0; off < got; off += 4)
	{
	  /* A pixel in the framebuffer is formatted as `%{blue}%{green}%{red}%{x}` in binary. */
	  r = buf[off + 2] & 255;
	  g = buf[off + 1] & 255;
	  b = buf[off + 0] & 255;
	  /* A pixel in the PNM image is formatted as `%{red} %{green} %{blue} ` in text. */
	  fprintf(file, "%s%s%s", inttable[r], inttable[g], inttable[b]);
	}
      
      /* If we read a whole number of pixels, reset the buffer, otherwise,
         move the unconverted bytes to the beginning of the buffer. */
      if (off != got)
	{
	  off -= 4;
	  memcpy(buf, buf + off, (got - off) * sizeof(char));
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
 * @param   fbno      The number of the framebuffer
 * @param   execname  `argv[0]` from `main`
 * @return            Zero on success, -1 on error
 */
static int save(const char* fbpath, const char* imgpath, int fbno, const char* execname)
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
      execlp("convert", "convert", DEVDIR "/stdin", imgpath, NULL);
      perror(execname);
      exit(1);
    }
  
  /* Parent process: */
  
  /* Close the read-end of the pipe. */
  close(pipe_rw[0]);
  
  /* Create a PNM-image of the framebuffer. */
  if (save_pnm(fbpath, fbno, pipe_rw[1]) < 0)
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
 * Take a screenshow of all framebuffers
 * 
 * @param   argc  The number of elements in `argv`
 * @param   argv  Command line arguments
 * @return        Zero on and only on success
 */
int main(int argc, char* argv[])
{
  char fbpath[PATH_MAX];
  char imgpath[PATH_MAX];
  int i, fbno;
  
  (void) argc;
  
  /* The a screenshot of each framebuffer. */
  for (fbno = 0;; fbno++)
    {
      /* Get pathname for framebuffer, and stop if we have read all existing ones. */
      sprintf(fbpath, DEVDIR "/fb%i", fbno);
      if (access(fbpath, F_OK))
	break;
      
      /* Get output pathname. */
      sprintf(imgpath, "fb%i.png", fbno);
      if (access(imgpath, F_OK) == 0)
	for (i = 2;; i++)
	  {
	    sprintf(imgpath, "fb%i.png.%i", fbno, i);
	    if (access(imgpath, F_OK))
	      break;
	  }
      
      /* Take a screenshot of the current framebuffer. */
      if (save(fbpath, imgpath, fbno, *argv) < 0)
	return perror(*argv), 1;
      fprintf(stderr, "Saved framebuffer %i to %s\n", fbno, imgpath);
    }
  
  return 0;
}

