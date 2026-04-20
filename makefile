###################################################################################
##
## GNU g++ makefile.in for the Atari++ emulator
##
## $Id: makefile.in,v 1.22 2020/07/18 16:32:40 thor Exp $
##
##
###################################################################################

.PHONY:		install uninstall distrib deb

CXX		= g++
MAKEFILE	= makefile
CFLAGS		=   -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -x c++ -c -Wall -W -Wunused -Wpointer-arith -pedantic -Wcast-qual -Wwrite-strings -Wno-long-long -Wredundant-decls -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wimplicit-fallthrough=3 #-no-rtti breaks exception handling

OPTIMIZER	= -O3 -DDEBUG_LEVEL=0 -DCHECK_LEVEL=0 -funroll-loops -fstrict-aliasing -Wno-redundant-decls -fexpensive-optimizations -fstrength-reduce -fgcse -fcse-follow-jumps -fcse-skip-blocks -frerun-cse-after-loop -frerun-loop-opt -fforce-addr
		  #-fschedule-insns # These give "register spill" errors.
		  #-fschedule-insns2 #-fomit-frame-pointer breaks exception handling for 3.2

VALGRIND	= -O3 -DDEBUG_LEVEL=0 -DCHECK_LEVEL=0 -funroll-loops -fstrict-aliasing -Wno-redundant-decls -fexpensive-optimizations -fstrength-reduce -fgcse -fcse-follow-jumps -fcse-skip-blocks -frerun-cse-after-loop -frerun-loop-opt -fforce-addr -ggdb3 -DVALGRIND
PROFILER	= -pg -ggdb3 -pg -fno-omit-frame-pointer -fno-inline
LDPROF		= -pg
DEBUGGER	= -ggdb3 -fno-inline -DDEBUG_LEVEL=2 -DCHECK_LEVEL=3
TOASM           = -S
DBLIBS          = #-lefence # does also break exception handling
LD		= g++
LDFLAGS		=   -lSM -lICE    -lSM -lICE  -lSDL
LDLIBS		=  -lSM -lICE  -lz -lXv -lX11 -lXext -lX11 -lasound -lm 
ECHO		= echo
prefix		= /usr/local
exec_prefix	= ${prefix}
datadir		= ${prefix}/share
BIN_PATH	= ${exec_prefix}/bin
MAN_PATH	= ${prefix}/share/man
###################################################################################

include Makefile.atari

install:
		mkdir -p $(DESTDIR)$(BIN_PATH)
		cp atari++ $(DESTDIR)$(BIN_PATH)
		chmod a+rx $(DESTDIR)$(BIN_PATH)/atari++
		mkdir -p $(DESTDIR)$(MAN_PATH)/man6
		gzip -9 <atari++.man >$(DESTDIR)$(MAN_PATH)/man6/atari++.6.gz
		chmod a+r $(DESTDIR)$(MAN_PATH)/man6/atari++.6.gz	
		mkdir -p $(DESTDIR)$(datadir)/doc/atari++
		cp COPYRIGHT $(DESTDIR)$(datadir)/doc/atari++/copyright
		chmod a+r $(DESTDIR)$(datadir)/doc/atari++/copyright

uninstall:
		rm $(DESTDIR)$(BIN_PATH)/atari++
		rm $(DESTDIR)$(MAN_PATH)/man6/atari++.6.gz

distrib:	$(SOURCES) $(INCLUDES)
		rm -f atari++.tgz
		$(MAKE) -f Makefile distrib

deb:		$(SOURCES) $(INCLUDES)
		$(MAKE) clean
		$(MAKE) -f Makefile deb
