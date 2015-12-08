# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


# The version of the package
VERSION = 1.1

# The package path prefix, if you want to install to another root, set DESTDIR to that root
PREFIX = /usr
# The command path excluding prefix
BIN = /bin
# The resource path excluding prefix
DATA = /share
# The command path including prefix
BINDIR = $(PREFIX)$(BIN)
# The resource path including prefix
DATADIR = $(PREFIX)$(DATA)
# The generic documentation path including prefix
DOCDIR = $(DATADIR)/doc
# The info manual documentation path including prefix
INFODIR = $(DATADIR)/info
# The man page documentation path including prefix
MANDIR = $(DATADIR)/man
# The man page section 1 path including prefix
MAN1DIR = $(MANDIR)/man1
# The locale path including prefix
LOCALEDIR = $(DATADIR)/locale
# The license base path including prefix
LICENSEDIR = $(DATADIR)/licenses

# The /dev directory that should be compiled into the program
DEVDIR = /dev
# The /sys directory that should be compiled into the program
SYSDIR = /sys

# The name of the command as it should be installed
COMMAND = scrotty
# The name of the package as it should be installed
PKGNAME = scrotty

# Programs the makefile uses.
#  Part of GNU Coreutils:
MKDIR ?= mkdir
MV ?= mv
RM ?= rm
RMDIR ?= rmdir
TRUE ?= true
TEST ?= test
ECHO ?= echo
PRINTF ?= printf
INSTALL ?= install
INSTALL_PROGRAM ?= $(INSTALL) -m755
INSTALL_DATA ?= $(INSTALL) -m644
INSTALL_DIR ?= $(INSTALL) -dm755
#  Part of Texinfo:
MAKEINFO ?= makeinfo
MAKEINFO_HTML ?= $(MAKEINFO) --html
#  Part of Texlive-plainextra:
TEXI2PDF ?= texi2pdf
TEXI2DVI ?= texi2dvi
TEXI2PS ?= texi2pdf --ps
#  Part of GCC:
CC ?= cc
CPP ?= cpp
#  Part of GNU Gettext:
XGETTEXT ?= xgettext
MSGFMT ?= msgfmt
MSGMERGE ?= msgmerge
MSGINIT ?= msginit

# Additional options for compiling PDF, DVI, and PS manuals.
TEXINFO_FLAGS =
# Optimisation settings for C code compilation
OPTIMISE = -O2
# Warnings settings for C code compilation
WARN = -Wall -Wextra -pedantic -Wdouble-promotion -Wformat=2 -Winit-self -Wmissing-include-dirs      \
       -Wtrampolines -Wmissing-prototypes -Wmissing-declarations -Wnested-externs                    \
       -Wno-variadic-macros -Wsync-nand -Wunsafe-loop-optimizations -Wcast-align                     \
       -Wdeclaration-after-statement -Wundef -Wbad-function-cast -Wwrite-strings -Wlogical-op        \
       -Wstrict-prototypes -Wold-style-definition -Wpacked -Wvector-operation-performance            \
       -Wunsuffixed-float-constants -Wsuggest-attribute=const -Wsuggest-attribute=noreturn           \
       -Wsuggest-attribute=format -Wnormalized=nfkc -fstrict-aliasing -fipa-pure-const -ftree-vrp    \
       -fstack-usage -funsafe-loop-optimizations -Wshadow -Wredundant-decls -Winline -Wcast-qual     \
       -Wsign-conversion -Wstrict-overflow=5 -Wconversion -Wsuggest-attribute=pure -Wswitch-default  \
       -Wstrict-aliasing=1 -fstrict-overflow -Wfloat-equal -Wpadded -Waggregate-return               \
       -Wtraditional-conversion
# The C standard for C code compilation
STD = -std=c99
# CPP flags
DEFS = -D'DEVDIR="$(DEVDIR)"' -D'SYSDIR="$(SYSDIR)"' -D'PACKAGE="$(PKGNAME)"'  \
       -D'LOCALEDIR="$(LOCALEDIR)"' -D'PROGRAM_VERSION="$(VERSION)"'
ifndef WITHOUT_GETTEXT
DEFS += -D'USE_GETTEXT=1'
endif

# List of translations
LOCALES = sv

# List of man page translations
MAN_LOCALES = sv

# Files generated texi2html
HTML_FILES = GNU-Free-Documentation-License.html index.html Invoking.html Overview.html



.PHONY: all
all: base info locale

.PHONY: everything
everything: base doc locale

.PHONY: base
base: cmd

.PHONY: cmd
cmd: bin/scrotty

obj/scrotty.o: src/scrotty.c
	@$(MKDIR) -p obj
	$(CC) $(STD) $(OPTIMISE) $(WARN) $(DEFS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

bin/scrotty: obj/scrotty.o
	@$(MKDIR) -p bin
	$(CC) $(STD) $(OPTIMISE) $(WARN) $(LDFLAGS) -o $@ $^

.PHONY: doc
doc: info pdf dvi ps html

.PHONY: info
info: bin/scrotty.info
bin/%.info: doc/info/%.texinfo doc/info/*.texinfo doc/info/*/*.texinfo
	@$(MKDIR) -p bin
	$(MAKEINFO) $<
	$(MV) $*.info $@

.PHONY: pdf
pdf: bin/scrotty.pdf
bin/%.pdf: doc/info/%.texinfo doc/info/*.texinfo doc/info/*/*.texinfo
	@$(MKDIR) -p obj/pdf bin
	cd obj/pdf && $(TEXI2PDF) ../../$< $(TEXINFO_FLAGS) < /dev/null
	$(MV) obj/pdf/$*.pdf $@

.PHONY: dvi
dvi: bin/scrotty.dvi
bin/%.dvi: doc/info/%.texinfo doc/info/*.texinfo doc/info/*/*.texinfo
	@$(MKDIR) -p obj/dvi bin
	cd obj/dvi && $(TEXI2DVI) ../../$< $(TEXINFO_FLAGS) < /dev/null
	$(MV) obj/dvi/$*.dvi $@

.PHONY: ps
ps: bin/scrotty.ps
bin/%.ps: doc/info/%.texinfo doc/info/*.texinfo doc/info/*/*.texinfo
	@$(MKDIR) -p obj/ps bin
	cd obj/ps && $(TEXI2PS) ../../$< $(TEXINFO_FLAGS) < /dev/null
	$(MV) obj/ps/$*.ps $@

.PHONY: html
html: bin/html/scrotty/index.html
bin/html/scrotty/index.html: doc/info/scrotty.texinfo doc/info/*.texinfo doc/info/*/*.texinfo
	@$(MKDIR) -p bin/html
	cd bin/html && $(MAKEINFO_HTML) ../../$< < /dev/null

ifdef WITHOUT_GETTEXT
.PHONY: locale
locale:
else
.PHONY: locale
locale: $(foreach L,$(LOCALES),bin/mo/$(L)/messages.mo)
endif

bin/mo/%/messages.mo: po/%.po
	@$(MKDIR) -p bin/mo/$*
	cd bin/mo/$* && $(MSGFMT) ../../../$<

obj/scrotty.pot: src/scrotty.c
	@$(MKDIR) -p obj
	$(CPP) -DUSE_GETTEXT=1 src/scrotty.c |  \
	$(XGETTEXT) -o "$@" -Lc --from-code utf-8 --package-name scrotty  \
	--package-version 1.1 --no-wrap --force-po  \
	--copyright-holder 'Mattias Andrée (maandree@member.fsf.org)' -

# Developers: run this to update .po files with new messages.
.PHONY: update-po
update-po: $(foreach L,$(LOCALES),po/$(L).po)

po/%.po: obj/scrotty.pot
	@$(MKDIR) -p po
	if ! $(TEST) -e $@; then  \
	$(MSGINIT) --no-translator --no-wrap -i $< -o $@ -l $*;  \
	else  \
	$(MSGMERGE) --no-wrap -U $@ $<;  \
	fi
	@touch $@

.PHONY: TAGS
TAGS:
	@echo "TAGS is not implemented. Did not see the need. Feel free to implement if you want."

.PHONY: tags
tags:
	@echo "tags is not implemented. Did not see the need. Feel free to implement if you want."


.PHONY: install
install: install-base install-info install-man install-locale

.PHONY: install-everything
install-everything: install-base install-doc install-locale

.PHONY: install-base
install-base: install-cmd install-copyright

.PHONY: install-strip
install-strip: install-base-strip install-info install-man install-locale

.PHONY: install-everything-strip
install-everything-strip: install-base-strip install-doc install-locale

.PHONY: install-base-strip
install-base-strip: install-cmd-strip install-copyright

.PHONY: install-cmd
install-cmd: bin/scrotty
	$(INSTALL_DIR) -- "$(DESTDIR)$(BINDIR)"
	$(INSTALL_PROGRAM) $< -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"

.PHONY: install-cmd-strip
install-cmd-strip: bin/scrotty
	$(INSTALL_DIR) -- "$(DESTDIR)$(BINDIR)"
	$(INSTALL_PROGRAM) -s $< -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"

.PHONY: install-copyright
install-copyright: install-copying install-license

.PHONY: install-copying
install-copying:
	$(INSTALL_DIR) -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	$(INSTALL_DATA) COPYING -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/COPYING"

.PHONY: install-license
install-license:
	$(INSTALL_DIR) -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	$(INSTALL_DATA) LICENSE -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/LICENSE"

.PHONY: install-doc
install-doc: install-info install-pdf install-ps install-dvi install-html install-man

.PHONY: install-info
install-info: bin/scrotty.info
	$(INSTALL_DIR) -- "$(DESTDIR)$(INFODIR)"
	$(INSTALL_DATA) $< -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"

.PHONY: install-pdf
install-pdf: bin/scrotty.pdf
	$(INSTALL_DIR) -- "$(DESTDIR)$(DOCDIR)"
	$(INSTALL_DATA) $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"

.PHONY: install-ps
install-ps: bin/scrotty.ps
	$(INSTALL_DIR) -- "$(DESTDIR)$(DOCDIR)"
	$(INSTALL_DATA) $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"

.PHONY: install-dvi
install-dvi: bin/scrotty.dvi
	$(INSTALL_DIR) -- "$(DESTDIR)$(DOCDIR)"
	$(INSTALL_DATA) $< -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"

.PHONY: install-html
install-html: $(foreach F,$(HTML_FILES),bin/html/scrotty/$(F))
	$(INSTALL_DIR) -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME)/html"
	$(INSTALL_DATA) $^ -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME)/html/"

.PHONY: install-man
install-man: install-man-untranslated install-man-locale

.PHONY: install-man-untranslated
install-man-untranslated:
	$(INSTALL_DIR) -- "$(DESTDIR)$(MAN1DIR)"
	$(INSTALL_DATA) doc/man/scrotty.1 -- "$(DESTDIR)$(MAN1DIR)/$(COMMAND).1"

.PHONY: install-man-locale
install-man-locale:
	$(foreach L,$(MAN_LOCALES),$(INSTALL_DIR) -- "$(DESTDIR)$(MANDIR)/$(L)/man1" &&) true
	$(foreach L,$(MAN_LOCALES),$(INSTALL_DATA) doc/man/scrotty.$(L).1 -- "$(DESTDIR)$(MANDIR)/$(L)/man1/$(COMMAND).1" &&) true

ifdef WITHOUT_GETTEXT
.PHONY: install-locale
install-locale:
else
.PHONY: install-locale
install-locale:
	$(INSTALL) -dm755 -- $(foreach L,$(LOCALES),"$(DESTDIR)$(LOCALEDIR)/$(L)/LC_MESSAGES")
	$(foreach L,$(LOCALES),$(INSTALL_DATA) bin/mo/$(L)/messages.mo -- "$(DESTDIR)$(LOCALEDIR)/$(L)/LC_MESSAGES/$(PKGNAME).mo" &&) $(TRUE)
endif


.PHONY: uninstall
uninstall:
	-$(RM) -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"
	-$(RM) -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/COPYING"
	-$(RM) -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/LICENSE"
	-$(RMDIR) -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	-$(RM) -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"
	-$(RM) -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"
	-$(RM) -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"
	-$(RM) -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"
	-$(RM) -- $(foreach F,$(HTML_FILES),"$(DESTDIR)$(DOCDIR)/$(PKGNAME)/html/$(F)")
	-$(RM) -- "$(DESTDIR)$(MAN1DIR)/$(COMMAND).1"
	-$(RM) -- $(foreach L,$(MAN_LOCALES),"$(DESTDIR)$(MANDIR)/$(L)/man1/$(COMMAND).1")
	-$(RM) -- $(foreach L,$(LOCALES),"$(DESTDIR)$(LOCALEDIR)/$(L)/LC_MESSAGES/$(PKGNAME).mo")


.PHONY: clean
clean:
	-$(RM) -r bin obj $(PKGNAME)-*.tar* $(PKGNAME)-*.checksums*

.PHONY: distclean
distclean: clean

.PHONY: mostlyclean
mostlyclean: clean

.PHONY: maintainer-clean
maintainer-clean: clean


# These rules are used for making releases

DIST_FMTS = tar tar.xz tar.bz2 tar.gz
ifndef DO_NOT_SIGN
DIST_FILES = $(foreach F,$(DIST_FMTS),$(PKGNAME)-$(VERSION).$(F) $(PKGNAME)-$(VERSION).$(F).sig)
else
DIST_FILES = $(foreach F,$(DIST_FMTS),$(PKGNAME)-$(VERSION).$(F))
endif
DIST_CHECKSUMS_ALL = cksum md2sum md4sum md5sum md6sum sha0sum sha1sum sha224sum  \
                     sha256sum sha384sum sha512sum sha512-224sum sha512-256sum \
                     sha3-224sum sha3-256sum sha3-384sum sha3-512sum shake256sum  \
                     shake512sum rawshake256sum rawshake512sum keccak-224sum  \
                     keccak-256sum keccak-384sum keccak-512sum keccaksum
DIST_CHECKSUMS = $(foreach C,$(DIST_CHECKSUMS_ALL),$(shell if command -v $(C) >/dev/null; then echo $(C); fi))

.PHONY: dist
dist: $(DIST_FILES) dist-checksums

ifndef DO_NOT_SIGN
.PHONY: dist-checksums
dist-checksums: $(PKGNAME)-$(VERSION).checksums $(PKGNAME)-$(VERSION).checksums.sig

.PHONY: dist-tar
dist-tar: $(PKGNAME)-$(VERSION).tar $(PKGNAME)-$(VERSION).tar.sig

.PHONY: dist-xz
dist-xz: $(PKGNAME)-$(VERSION).tar.xz $(PKGNAME)-$(VERSION).tar.xz.sig

.PHONY: dist-bz2
dist-bz2: $(PKGNAME)-$(VERSION).tar.bz2 $(PKGNAME)-$(VERSION).tar.bz2.sig

.PHONY: dist-gz
dist-gz: $(PKGNAME)-$(VERSION).tar.gz $(PKGNAME)-$(VERSION).tar.sig
else
.PHONY: dist-checksums
dist-checksums: $(PKGNAME)-$(VERSION).checksums

.PHONY: dist-tar
dist-tar: $(PKGNAME)-$(VERSION).tar

.PHONY: dist-xz
dist-xz: $(PKGNAME)-$(VERSION).tar.xz

.PHONY: dist-bz2
dist-bz2: $(PKGNAME)-$(VERSION).tar.bz2

.PHONY: dist-gz
dist-gz: $(PKGNAME)-$(VERSION).tar.gz
endif

$(PKGNAME)-$(VERSION).tar:
	@if $(TEST) -f $@; then $(RM) $@; fi
	git archive --prefix=$(PKGNAME)-$(VERSION)/ --format=tar $(VERSION) -o $@

$(PKGNAME)-$(VERSION).tar.xz: $(PKGNAME)-$(VERSION).tar
	@if $(TEST) -f $@; then $(RM) $@; fi
	xz -ke9 $<

$(PKGNAME)-$(VERSION).tar.bz2: $(PKGNAME)-$(VERSION).tar
	@if $(TEST) -f $@; then $(RM) $@; fi
	bzip2 -k9 $<

$(PKGNAME)-$(VERSION).tar.gz: $(PKGNAME)-$(VERSION).tar
	@if $(TEST) -f $@; then $(RM) $@; fi
	gzip -k9 $<

$(PKGNAME)-$(VERSION).checksums: $(DIST_FILES)
	@$(ECHO) Generating $@
	@$(PRINTF) '' > $@
	@if ! ($(ECHO) ':: sum -r ::' && sum -r $^ && $(ECHO)) >> $@ ; then $(PRINTF) '' > $@; fi
	@if ! ($(ECHO) ':: sum -s ::' && sum -s $^ && $(ECHO)) >> $@ ; then $(PRINTF) '' > $@; fi
	@$(foreach C,$(DIST_CHECKSUMS),$(ECHO) ':: $(C) ::' >> $@ && $(C) $^ | grep -v '^--' >> $@ && $(ECHO) >> $@ &&) $(TRUE)

%.sig: %
	@if $(TEST) -f $@; then $(RM) $@; fi
	gpg $(GPG_FLAGS) --local-user $(GPG_KEY) --detach-sign --armor --output $@ < $<

