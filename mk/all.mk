# Copyright (C) 2015  Mattias Andrée <maandree@member.fsf.org>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


#=== This file includes all the other files in appropriate order. ===#

ifndef Q
A = \e[35m
Z = [m
endif

include mk/path.mk
include mk/empty.mk
include mk/tools.mk
include mk/copy.mk
include mk/lang-c.mk
include mk/texinfo.mk
include mk/man.mk
include mk/i18n.mk
include mk/clean.mk
include mk/dist.mk
include mk/tags.mk

