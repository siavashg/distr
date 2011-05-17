#ifndef SERVER_H
#define SERVER_H

#include "queue.h"

#define USERNAME_MAX_LENGTH 128

struct client_node {
	int fd;

	struct bufferevent *bufev;
	struct sockaddr_in addr;
	int port;

	char username[USERNAME_MAX_LENGTH];

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

int check_auth(struct client_node *client_node);

int write_node(struct client_node *client_node, const char *s);
int write_node_end(struct client_node *client_node, const char *s);
void write_node_end_cb(struct bufferevent *bev, void *arg);

void log_recv(struct client_node *client_node, const char *s);
void log_send(struct client_node *client_node, const char *s);

#endif
