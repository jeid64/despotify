OBJS = simple.o

LIBDIR = ../../lib
LIB = $(LIBDIR)/libdespotify.la

CFLAGS += -I$(LIBDIR) -std=c99

all: simple

# These are the files we depgen for. :-)
CFILES = $(OBJS:.o=.c)
include ../depgen.mk

simple: $(OBJS) $(LIB)
	@echo LD $@
	$(SILENT)$(LT) --mode=link $(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIB)

clean:
	$(LT) --mode=clean rm -f simple
	rm -f $(OBJS) Makefile.dep

install: simple
	@echo "Copying simple binary to /usr/bin/simple"
	$(LT) --mode=install install simple /usr/bin/simple

uninstall:
	@echo "Removing simple..."
	rm -f /usr/bin/simple
