# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


# The package path prefix, if you want to install to another root, set DESTDIR to that root
PREFIX ?= /usr
# The command path excluding prefix
BIN ?= /bin
# The resource path excluding prefix
DATA ?= /share
# The command path including prefix
BINDIR ?= $(PREFIX)$(BIN)
# The resource path including prefix
DATADIR ?= $(PREFIX)$(DATA)
# The generic documentation path including prefix
DOCDIR ?= $(DATADIR)/doc
# The info manual documentation path including prefix
INFODIR ?= $(DATADIR)/info
# The license base path including prefix
LICENSEDIR ?= $(DATADIR)/licenses

# The /dev directory that should be compiled into the program
DEVDIR = /dev
# The /sys directory that should be compiled into the program
SYSDIR = /sys

# The name of the command as it should be installed
COMMAND ?= scrotty
# The name of the package as it should be installed
PKGNAME ?= scrotty

# Optimisation settings for C code compilation
OPTIMISE ?= -Og -g
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
STD = -std=gnu99
# CPP flags
DEFS = -D'DEVDIR="$(DEVDIR)"' -D'SYSDIR="$(SYSDIR)"'



.PHONY: all
all: cmd

.PHONY: cmd
cmd: bin/scrotty

obj/scrotty.o: src/scrotty.c
	@mkdir -p obj
	$(CC) $(STD) $(OPTIMISE) $(WARN) $(DEFS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

bin/scrotty: obj/scrotty.o
	@mkdir -p bin
	$(CC) $(STD) $(OPTIMISE) $(WARN) $(LDFLAGS) -o $@ $^


.PHONY: install
install: install-base

.PHONY: install-all
install-all: install-base

.PHONY: install-base
install-base: install-cmd install-copyright

.PHONY: install-cmd
install-cmd: bin/scrotty
	install -dm755 -- "$(DESTDIR)$(BINDIR)"
	install -m755 $< -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"

.PHONY: install-copyright
install-copyright: install-copying install-license

.PHONY: install-copying
install-copying:
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PACKAGE)"
	install -m644 COPYING -- "$(DESTDIR)$(LICENSEDIR)/$(PACKAGE)/COPYING"

.PHONY: install-license
install-license:
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PACKAGE)"
	install -m644 LICENSE -- "$(DESTDIR)$(LICENSEDIR)/$(PACKAGE)/LICENSE"


.PHONY: uninstall
uninstall:
	-rm -- "$(DESTDIR)$(BINDIR)/$(COMMAND)"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PACKAGE)/COPYING"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PACKAGE)/LICENSE"
	-rmdir -- "$(DESTDIR)$(LICENSEDIR)/$(PACKAGE)"


.PHONY: clean
clean:
	-rm -r bin obj scrotty.{info,pdf,ps,dvi} *.su src/*.su

