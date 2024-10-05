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
#if !defined(_POSIX_C_SOURCE) || (_POSIX_C_SOURCE < 200809L)
# if defined(_POSIX_C_SOURCE)
#  undef _POSIX_C_SOURCE
# endif
# define _POSIX_C_SOURCE  200809L
#endif
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>


#ifdef USE_GETTEXT
# include <libintl.h>
# define _(STR)  gettext (STR)
#else
# define _(STR)  STR
#endif


#ifndef DEVDIR
# define DEVDIR  "/dev"
#endif
#ifndef SYSDIR
# define SYSDIR  "/sys"
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


/**
 * `argv[0]` from `main`.
 */
extern const char *execname;

/**
 * If a function fails when it tries to
 * open a file, it will set this variable
 * point to the pathname of that file.
 */
extern const char *failure_file;

