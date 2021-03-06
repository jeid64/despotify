#
# $Id$
# 
#

export

CFLAGS = -Wall -Wextra -ggdb -std=gnu99
LDFLAGS = -lz -lvorbisfile -pthread

LD = $(CC)

ifndef V
# run "make V=1" to see full command lines
SILENT := @
SILENTDIR := -s
SILENTLIB := --quiet
endif

INSTALL_PREFIX ?= ${DESTDIR}/usr
LINUX_BACKEND = gstreamer

include Makefile.local.mk

ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG
endif

ifeq ($(MP3_SUPPORT), 1)
    CFLAGS += -DMP3_SUPPORT
    LDFLAGS += -lmpg123
endif

# Mac OS X specifics
ifeq ($(shell uname -s),Darwin)
    LT = glibtool --tag=CC
else
    LT = libtool $(SILENTLIB) --tag=CC
endif

# windows specifics
ifeq ($(firstword $(subst _, ,$(shell uname -s))), MINGW32)
    LDFLAGS += -leay32
else
    LDFLAGS += -lcrypto
endif

# gcc2 & 3 are ancient and doesn't support -Wno-pointer-sign
ifeq ($(firstword $(subst ., ,$(shell $(CC) -dumpversion))), 4)
    CFLAGS += -Wno-pointer-sign
endif

CLIENTS = clients/simple
ifeq ($(CLIENT_DESPOTIFY), 1)
    CLIENTS += clients/despotify
endif
ifeq ($(CLIENT_GATEWAY), 1)
	CLIENTS += clients/gateway
endif
ifeq ($(CLIENT_MAEMIFY), 1)
	CLIENTS += clients/maemify
endif

SUBDIRS = lib $(CLIENTS)

.PHONY: all clean $(SUBDIRS) install uninstall

all: $(SUBDIRS)

clean: 
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f local.mk clean || exit $$?; \
	done

clients: $(CLIENTS)
$(CLIENTS): lib
$(SUBDIRS):
	$(MAKE) $(SILENTDIR) -C $@ -f local.mk

install: $(SUBDIRS)
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f local.mk install || exit $$?; \
	done

uninstall:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir -f local.mk uninstall || exit $$?; \
	done

Makefile.local.mk:
	@echo " **** No Makefile.local.mk found, copying dist."
	cp Makefile.local.mk.dist Makefile.local.mk
