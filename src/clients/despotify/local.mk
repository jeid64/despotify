#
# $Id$
# 

unexport LDFLAGS CFLAGS

LIBDIR = ../../lib
LIB = $(LIBDIR)/libdespotify.la

NCURSES_INCLUDE ?= /usr/include/ncursesw

CFLAGS += -I$(LIBDIR) -I$(NCURSES_INCLUDE)
LDFLAGS += -lncursesw

DESPOTIFY_OBJS = commands.o event.o main.o session.o ui.o ui_footer.o ui_help.o ui_log.o ui_sidebar.o ui_tracklist.o

.PHONY: all clean install uninstall
all: despotify 

# These are the files we depgen for. :-)
CFILES = $(DESPOTIFY_OBJS:.o=.c)
include ../depgen.mk

%.o: %.c
	@echo CC $<
	$(SILENT)$(CC) $(CFLAGS) -o $@ -c $<

despotify: $(DESPOTIFY_OBJS) $(LIB)
	@echo LD $@
	$(SILENT)$(LT) --mode=link $(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(LIB) $(DESPOTIFY_OBJS)

clean:
	$(LT) --mode=clean rm -f despotify
	rm -f $(DESPOTIFY_OBJS) Makefile.dep

install: despotify 
	@echo "Copying despotify binary to $(INSTALL_PREFIX)/bin/despotify"
	$(LT) --mode=install install despotify $(INSTALL_PREFIX)/bin/despotify

uninstall:
	@echo "Removing despotify..."
	rm -f $(INSTALL_PREFIX)/bin/despotify
