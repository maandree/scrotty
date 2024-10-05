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
#ifndef PROGRAM_NAME
# define PROGRAM_NAME  "scrotty"
#endif
#ifndef PROGRAM_VERSION
# define PROGRAM_VERSION  "development-version"
#endif



/**
 * Print usage information.
 * 
 * @return  Zero on success, -1 on error.
 */
int print_help (void);

/**
 * Print program name and version.
 * 
 * @return  Zero on success, -1 on error.
 */
int print_version (void);

/**
 * Print copyright information.
 * 
 * @return  Zero on success, -1 on error.
 */
int print_copyright (void);

