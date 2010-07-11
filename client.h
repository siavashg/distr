#ifndef CLIENT_H
#define CLIENT_H

#include "queue.h"

struct client {
	int fd;

	struct bufferevent *buf_ev;
	struct sockaddr_in client_addr;

	char username[32];
	int port;

	TAILQ_ENTRY(client) entries;
};
TAILQ_HEAD(, client) clients;


void bufev_on_read(struct bufferevent *bev, void *arg);
void bufev_on_write(struct bufferevent *bev, void *arg);
void bufev_on_error(struct bufferevent *bev, short what, void *arg);
void bufev_on_accept(int fd, short ev, void *arg);
int setnonblock(int fd);

void log_send(struct client *client, const char *s);
void log_recv(struct client *client, const char *s);


#endif
