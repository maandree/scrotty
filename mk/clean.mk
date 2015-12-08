# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== These rules are used for cleaning the repository of generated files. ===#


# Delete all files are normally created during a build.
.PHONY: clean
clean:
	@$(PRINTF_INFO) '\e[00;01;31mCLEANING\e[34m\e[00m\n'
	-$(Q)$(RM) -r bin obj $(PKGNAME)-*.tar* $(PKGNAME)-*.checksums*
	@$(ECHO)

# Delete all files that are created during configuration.
# Note, this, for some reason, should not imply `make clean`.
.PHONY: distclean
distclean:

# Like `make clean` but do not remove massive binaries
# that are seldom recompiled.
.PHONY: mostlyclean
mostlyclean: clean

# Delete everything except ./configure
.PHONY: maintainer-clean
maintainer-clean: clean distclean
	@$(ECHO)
	@$(ECHO) 'This command is intended for maintainers to use; it'
	@$(ECHO) 'deletes files that may need special tools to rebuild.'
	@$(ECHO)

