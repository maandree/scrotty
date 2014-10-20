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
static const char* inttable[] =
  {
    LIST_0_9(""),  LIST_0_9("1"), LIST_0_9("2"), LIST_0_9("3"), LIST_0_9("4"),
    LIST_0_9("5"), LIST_0_9("6"), LIST_0_9("7"), LIST_0_9("8"), LIST_0_9("9"),
    
    LIST_0_9("10"), LIST_0_9("11"), LIST_0_9("12"), LIST_0_9("13"), LIST_0_9("14"),
    LIST_0_9("15"), LIST_0_9("16"), LIST_0_9("17"), LIST_0_9("18"), LIST_0_9("19"),
    
    LIST_0_9("20"), LIST_0_9("21"), LIST_0_9("22"), LIST_0_9("23"), LIST_0_9("24"),
    "250\n", "251\n", "252\n", "253\n", "254\n", "255\n"
  };


static int save_pnm(const char* fbpath, int fbno, int fd)
{
  char buf[PATH_MAX];
  FILE* file;
  int saved_errno, sizefd, fbfd, r, g, b;
  ssize_t got, off;
  
  sprintf(buf, SYSDIR "/class/graphics/fb%i/virtual_size", fbno);
  if (sizefd = open(buf, O_RDONLY), sizefd < 0)
    return -1;
  
  if (got = read(sizefd, buf, sizeof(buf) / sizeof(char) - 1), got < 0)
    return saved_errno = errno, close(sizefd), errno = saved_errno, -1;
  close(sizefd);
  buf[got] = '\0';
  *strchr(buf, ',') = ' ';
  
  fbfd = open(fbpath, O_RDONLY);
  if (fbfd < 0)
    return -1;
  
  file = fdopen(fd, "w");
  if (file == NULL)
    return saved_errno = errno, close(fbfd), errno = saved_errno, -1;
  fprintf(file, "P3\n%s255\n", buf);
  
  for (off = 0;;)
    {
      got = read(fbfd, buf + off, sizeof(buf) - off * sizeof(char));
      if (got < 0)
	return saved_errno = errno, fclose(file), close(fbfd), errno = saved_errno, -1;
      if (got += off, got == 0)
	break;
      
      for (off = 0; off < got; off += 4)
	{
	  r = buf[off + 2] & 255;
	  g = buf[off + 1] & 255;
	  b = buf[off + 0] & 255;
	  fprintf(file, "%s%s%s", inttable[r], inttable[g], inttable[b]);
	}
      if (off != got)
	{
	  off -= 4;
	  memcpy(buf, buf + off, (got - off) * sizeof(char));
	  off = got - off;
	}
      else
	off = 0;
    }
  
  fflush(file);
  fclose(file);
  close(fbfd);
  return 0;
}


static int save(const char* fbpath, const char* imgpath, int fbno, const char* execname)
{
  int pipe_rw[2];
  pid_t pid, reaped;
  int saved_errno, status;
  
  if (pipe(pipe_rw) < 0)
    return -1;
  
  if (pid = fork(), pid == -1)
    return saved_errno = errno, close(pipe_rw[0]), close(pipe_rw[1]), errno = saved_errno, -1;
  
  if (pid == 0)
    {
      close(pipe_rw[1]);
      if (pipe_rw[0] != STDIN_FILENO)
	{
	  close(STDIN_FILENO);
	  dup2(pipe_rw[0], STDIN_FILENO);
	  close(pipe_rw[0]);
	}
      execlp("convert", "convert", DEVDIR "/stdin", imgpath, NULL);
      perror(execname);
      exit(1);
    }
  
  close(pipe_rw[0]);
  
  if (save_pnm(fbpath, fbno, pipe_rw[1]) < 0)
    return saved_errno = errno, close(pipe_rw[1]), errno = saved_errno, -1;
  
  close(pipe_rw[1]);
  
  reaped = 0;
  while (reaped != pid)
    {
      reaped = waitpid(pid, &status, 0);
      if (reaped < 0)
	return -1;
    }
  
  return status == 0 ? 0 : -1;
}


int main(int argc, char* argv[])
{
  char fbpath[PATH_MAX];
  char imgpath[PATH_MAX];
  int i, fbno;
  
  (void) argc;
  
  for (fbno = 0;; fbno++)
    {
      sprintf(fbpath, DEVDIR "/fb%i", fbno);
      if (access(fbpath, F_OK))
	break;
      
      sprintf(imgpath, "fb%i.png", fbno);
      if (access(imgpath, F_OK) == 0)
	for (i = 2;; i++)
	  {
	    sprintf(imgpath, "fb%i.png.%i", fbno, i);
	    if (access(imgpath, F_OK))
	      break;
	  }
      
      if (save(fbpath, imgpath, fbno, *argv) < 0)
	return perror(*argv), 1;
      fprintf(stderr, "Saved framebuffer %i to %s\n", fbno, imgpath);
    }
  
  return 0;
}

