#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <ctype.h>

#include <sys/time.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>

#include <event.h>

#include "queue.h"
#include "distr.h"
#include "client.h"


struct client *new_client(struct client *client) {
	struct evbuffer *evbuffer;

	evbuffer = evbuffer_new();
	TAILQ_INSERT_TAIL(&clients, client, entries);

	evbuffer_add_printf(evbuffer, "WELCOME\n");
	evbuffer_write(evbuffer, client->fd);

	evbuffer_free(evbuffer);

	dprintf ("Connected (%s) (fd: %d)\n", inet_ntoa(client->client_addr.sin_addr), client->fd);
}

