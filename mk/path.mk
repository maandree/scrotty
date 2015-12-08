# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== This file define path variables. ===#


ifndef __PATH_MK_INCLUDED
__PATH_MK_INCLUDED = 1


# DIRECTORIES:

# The package path prefix, if you want to install to another root, set DESTDIR to that root.
PREFIX = /usr
# The command path excluding prefix.
BIN = /bin
# The administration command path excluding prefix.
SBIN = /sbin
# The library path excluding prefix.
LIB = /lib
# The executable library path excluding prefix.
LIBEXEC = /libexec
# The header-file path excluding prefix.
INCLUDE = /include
# The resource path excluding prefix.
DATA = /share
# The man page sections path excluding prefix.
MAN0 = /man0
MAN1 = /man1
MAN2 = /man2
MAN3 = /man3
MAN4 = /man4
MAN5 = /man5
MAN6 = /man6
MAN7 = /man7
MAN8 = /man8
MAN9 = /man9

# The command path including prefix.
BINDIR = $(PREFIX)$(BIN)
# The administration command path including prefix.
SBINDIR = $(PREFIX)$(SBIN)
# The library path including prefix.
LIBDIR = $(PREFIX)$(LIB)
# The executable library path including prefix.
LIBEXECDIR = $(PREFIX)$(LIBEXEC)
# The header-file path including prefix.
INCLUDEDIR = $(PREFIX)$(INCLUDE)
# The resource path including prefix.
DATADIR = $(PREFIX)$(DATA)
# The architecture-dependent resource path including prefix.
SYSDEPDATA = $(DATADIR)
# The generic documentation path including prefix.
DOCDIR = $(DATADIR)/doc
# The info manual documentation path including prefix.
INFODIR = $(DATADIR)/info
# The DVI documentation path including prefix.
DVIDIR = $(DOCDIR)
# The PDF documentation path including prefix.
PDFDIR = $(DOCDIR)
# The PostScript documentation path including prefix.
PSDIR = $(DOCDIR)
# The HTML documentation path including prefix.
HTMLDIR = $(DOCDIR)
# The man page documentation path including prefix.
MANDIR = $(DATADIR)/man
# The locale path including prefix.
LOCALEDIR = $(DATADIR)/locale
# The license base path including prefix.
LICENSEDIR = $(DATADIR)/licenses
# The persistent variable data directory.
VARDIR = /var
# The persistent directory for temporary files.
VARTMPDIR = $(VARDIR)/tmp
# The network-common persistent variable data directory.
COMDIR = /com
# The network-common persistent directory for temporary files.
COMTMPDIR = $(COMDIR)/tmp
# The transient directory for temporary files.
TMPDIR = /tmp
# The transient directory for runtime files.
RUNDIR = /run
# The directory for site-specific configurations.
SYSCONFDIR = /etc
# The directory for pseudo-devices.
DEVDIR = /dev
# The /sys directory.
SYSDIR = /sys
# The /proc directory
PROCDIR = /proc
# The /proc/self directory
SELFPROCDIR = $(PROCDIR)/self
# The cache directory.
CACHEDIR = $(VARDIR)/cache
# The spool directory.
SPOOLDIR = $(VARDIR)/spool
# The empty directory.
EMPTYDIR = $(VARDIR)/empty
# The logfile directory.
LOGDIR = $(VARDIR)/log
# The state directory.
STATEDIR = $(VARDIR)/lib
# The highscore directory.
GAMEDIR = $(VARDIR)/games
# The lockfile directory.
LOCKDIR = $(RUNDIR)/lock
# The user skeleton directory.
SKELDIR = $(SYSCONFDIR)/skel
# The network-common cache directory.
COMCACHEDIR = $(COMDIR)/cache
# The network-common spool directory.
COMSPOOLDIR = $(COMDIR)/spool
# The network-common empty directory.
COMEMPTYDIR = $(COMDIR)/empty
# The network-common logfile directory.
COMLOGDIR = $(COMDIR)/log
# The network-common state directory.
COMSTATEDIR = $(COMDIR)/lib
# The network-common highscore directory.
COMGAMEDIR = $(COMDIR)/games


# FILENAME SUFFIXES:

# Filename suffixes for man pages by section.
MAN0EXT = 0
MAN1EXT = 1
MAN2EXT = 2
MAN3EXT = 3
MAN4EXT = 4
MAN5EXT = 5
MAN6EXT = 6
MAN7EXT = 7
MAN8EXT = 8
MAN9EXT = 9


# HELP VARIABLES:

# All path variables that includes the prefix,
# or are unaffected by the prefix.
_ALL_DIRS = BINDIR SBINDIR LIBDIR LIBEXECDIR INCLUDEDIR DATADIR SYSDEPDATA DOCDIR  \
            INFODIR DVIDIR PDFDIR PSDIR HTMLDIR MANDIR LOCALEDIR LICENSEDIR VARDIR  \
            VARTMPDIR COMDIR COMTMPDIR TMPDIR RUNDIR SYSCONFDIR DEVDIR SYSDIR  \
            PROCDIR SELFPROCDIR CACHEDIR SPOOLDIR EMPTYDIR LOGDIR STATEDIR GAMEDIR \
            LOCKDIR SKELDIR COMCACHEDIR COMSPOOLDIR COMEMPTYDIR COMLOGDIR COMSTATEDIR \
            COMGAMEDIR


endif

