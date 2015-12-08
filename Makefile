# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


### See INSTALL for information on how to install this package. ###


# Prototype directory paths. You can change them!
include mk/path.mk



# The name of the command as it should be installed.
COMMAND = scrotty
# The name of the package as it should be installed.
PKGNAME = scrotty

# Additional options for compiling DVI, PDF, and PostScript manuals.
TEXINFO_FLAGS =

# List of translations.
LOCALES = sv

# List of man page translations.
MAN_LOCALES = sv



# YOU, AS A USER, SHOULD NOT CHANGE THESE VARIABLES. {{

# Package information.
_PROJECT = scrotty
_VERSION = 1.1

# Used by mk/lang-c.mk
_C_STD = c99
_PEDANTIC = yes
_BIN = scrotty
_OBJ_scrotty = scrotty
_HEADER_DIRLEVELS = 0
_CPPFLAGS = -D'PACKAGE="$(PKGNAME)"' -D'PROGRAM_VERSION="$(_VERSION)"'

# Used by mk/i18n.mk
_SRC = $(foreach B,$(_BIN),$(_OBJ_$(B)))
_PROJECT_FULL = scrotty
_COPYRIGHT_HOLDER = Mattias Andrée (maandree@member.fsf.org)

# Used by mk/texinfo.mk
_TEXINFO_DIRLEVELS = 2
_INFOPARTS = 0
_HAVE_TEXINFO_MANUAL = yes
_HTML_FILES = Free-Software-Needs-Free-Documentation.html  GNU-Free-Documentation-License.html  \
              GNU-General-Public-License.html  index.html  Invoking.html  Overview.html  strftime.html

# Used by mk/man.mk
_MAN_PAGE_SECTIONS = 1
_MAN_1 = scrotty
_MAN_sv_1 = scrotty

# Used by mk/copy.mk
_COPYING = COPYING
_LICENSE = LICENSE

# }}


# All of the make rules.
include mk/all.mk

# In case you want add some configurations.
# Primarily intended for maintainers.
# Perhaps add GPG_KEY here.
-include .make-configurations

