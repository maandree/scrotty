# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== This file defines variables for all used commands. ===#


# Part of GNU Coreutils:
MKDIR ?= mkdir
CP ?= cp
MV ?= mv
RM ?= rm
RMDIR ?= rmdir
TRUE ?= true
TEST ?= test
TOUCH ?= touch
ECHO ?= echo
CUT ?= cut
TAC ?= tac
TAIL ?= tail
HEAD ?= head
SORT ?= sort
UNIQ ?= uniq
PRINTF ?= printf
WC ?= wc
INSTALL ?= install
INSTALL_PROGRAM ?= $(INSTALL) -m755
INSTALL_DATA ?= $(INSTALL) -m644
INSTALL_DIR ?= $(INSTALL) -dm755

# Part of GNU Findutils:
FIND ?= find
XARGS ?= xargs

# Part of GNU Grep:
GREP ?= grep

# Part of GNU Sed:
SED ?= sed

# Part of GNU Privacy Guard:
GPG ?= gpg

# Part of Texinfo:
MAKEINFO ?= makeinfo
MAKEINFO_HTML ?= $(MAKEINFO) --html

# Part of Texlive-plainextra:
TEXI2PDF ?= texi2pdf
TEXI2DVI ?= texi2dvi
TEXI2PS ?= texi2pdf --ps

# Part of Texlive-core:
PS2EPS ?= ps2eps

# Part of librsvg:
RSVG_CONVERT ?= rsvg-convert
SVG2PS ?= $(RSVG_CONVERT) --format=ps
SVG2PDF ?= $(RSVG_CONVERT) --format=pdf

# Part of GCC:
CC ?= cc
CPP ?= cpp

# Part of GNU Gettext:
XGETTEXT ?= xgettext
MSGFMT ?= msgfmt
MSGMERGE ?= msgmerge
MSGINIT ?= msginit

# Part of gzip:
GZIP ?= gzip
GZIP_COMPRESS ?= $(GZIP) -k9

# Part of bzip2:
BZIP2 ?= bzip2
BZIP2_COMPRESS ?= $(BZIP2) -k9

# Part of xz:
XZ ?= xz
XZ_COMPRESS ?= $(XZ) -ke9


# Change to $(TRUE) to suppress the bold red and blue output.
ifndef PRINTF_INFO
PRINTF_INFO = $(PRINTF)
endif

# Change to $(TRUE) to suppress empty lines
ifndef ECHO_EMPTY
ECHO_EMPTY = $(ECHO)
endif

