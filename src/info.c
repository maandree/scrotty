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
#include "info.h"



/**
 * Print usage information.
 * 
 * @return  Zero on success, -1 on error.
 */
int
print_help (void)
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
int
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
int
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

