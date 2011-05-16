#ifndef SERVER_H
#define SERVER_H

#include "queue.h"

struct client_node {
	int fd;

	struct bufferevent *bufev;
	struct sockaddr_in addr;

	char username[32];
	int port;

	TAILQ_ENTRY(client_node) entries;
};
TAILQ_HEAD(, client_node) client_nodes;


/**
 * Server functions
 */ 
int setnonblock(int fd);

int server_init(int port);
int server_shutdown(int server_fd);

void client_connect(struct client_node *client_node);
void client_disconnect(struct client_node *client_node);

void log_send(struct client_node *client_node, const char *s);
void log_recv(struct client_node *client_node, const char *s);

#endif
