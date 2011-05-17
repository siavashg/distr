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

OBJ = distr.o \
server.o \
server_events.o \
protocol.o

distr: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

test: test/test.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf distr *.o *~ distr.dSYM
