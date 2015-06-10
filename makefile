sources = $(wildcard *.c)
headers = $(wildcard *.h)
archive = archive.tar.bz2
defines = -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_FORTIFY_SOURCE=2
manpage = vcdmerge.man
exe_name = vcdmerge

prefix ?= /usr/local
exec_prefix ?= $(prefix)
datarootdir ?= $(prefix)/share
bindir ?= $(exec_prefix)/bin
infodir ?= $(datarootdir)/info
mandir ?= $(datarootdir)/man

INSTALL ?= install
CC ?= cc
INSTALL_PROGRAM ?= $(INSTALL)
INSTALL_DATA ?= $(INSTALL) -m 664

opt?=-O0

CFLAGS ?= -g $(opt) -fno-common -fstrict-aliasing -fstack-protector -Wall -Wextra
ALL_CFLAGS = -std=gnu99 $(defines) $(include_dirs) $(CFLAGS)
LIBRARIES = 

LDFLAGS ?= 
ALL_LDFLAGS = $(lib_dirs) $(LIBRARIES) $(LDFLAGS)
CPPFLAGS ?= 

.PHONY : all clean distclean install uninstall indent test

all : main

.c.o:
	$(CC) -c $(CPPFLAGS) $(ALL_CFLAGS) $<

indent :
	indent -nut -i2 -lp -bli0 *.c *.h

$(archive) : clean $(sources) $(headers) makefile
	tar -vcjf $(archive) $(wildcard *)

install : main vcdmerge.man
	$(INSTALL_PROGRAM) -T ./main "$(DESTDIR)$(bindir)/$(exe_name)"
	$(INSTALL_DATA) vcdmerge.man "$(DESTDIR)$(mandir)/man1/vcdmerge.1"

uninstall :
	rm -f "$(DESTDIR)$(mandir)/man1/vcdmerge.1"
	rm -f "$(DESTDIR)$(bindir)/$(exe_name)"

main : $(sources:.c=.o)
	$(CC) $(sources:.c=.o) $(ALL_CFLAGS) $(ALL_LDFLAGS) -o main 

%.d : %.c makefile
	cc -MM -MG $(include_dirs) $< > $@.$$$$; \
sed 's,\($*\)\.o[ :]*,\1.o $@\: ,g; s,[^\\]$$,& makefile,' < $@.$$$$ > $@; \
rm -f $@.$$$$

help.h: vcdmerge.man
	./gen_help_h.sh

ifneq ($(MAKECMDGOALS), clean)
ifneq ($(MAKECMDGOALS), distclean)

include $(sources:.c=.d)

endif
endif

tests=$(wildcard *.tmk)

ifeq ($(MAKECMDGOALS), test)
include $(tests)

test: $(tests:.tmk=.t)

endif

distclean :clean
	rm -f ./main

clean :
	rm -f $(sources:.c=.d)
	rm -f $(sources:.c=.o)
	rm -f $(tests:.tmk=.t)
	rm -f *~
	rm -f *.log
	rm -f c.vcd
	rm -f c2.vcd
	rm -f *.bak
	rm -f help.h

