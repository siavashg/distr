LIBEVENT = /usr/lib
CFLAGS =	-I. -I$(LIBEVENT) -Wall -g
LIBS =		-L$(LIBEVENT) -levent

distr_SOURES = distr.c \
		client.c

distr: distr.c client.c
	$(CC) $(CFLAGS) -o $@ distr.c client.c $(LIBS)

clean:
	rm -rf distr *~
