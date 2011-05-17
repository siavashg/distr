all: distr
.PHONY: distr

UNAME := $(shell uname)

CFLAGS	+= -I./include -Wall -g
LDLIBS	+= -levent

#
# Libevent
#
ifeq ($(UNAME), Linux)
LIBEVENT_PATH=/usr/lib/
CFLAGS	+= -I$(LIBEVENT_PATH)
LDFLAGS	+= -L$(LIBEVENT_PATH)
endif

ifeq ($(UNAME), Darwin)
CFLAGS	+= $(shell pkg-config --cflags libevent)
LDFLAGS	+= $(shell pkg-config --libs-only-L libevent)
endif

SOURCES = distr.c \
		server.c \
		server_events.c \
		protocol.c

distr: distr.c client.c
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf distr *~ distr.dSYM
