#ifndef SERVER_H
#define SERVER_H

#include "queue.h"

TAILQ_HEAD(, distr_node) client_nodes;

/**
 * Server functions
 */ 
int setnonblock(int fd);

int server_init(int port);
int server_shutdown(int server_fd);

void client_connect(struct distr_node *client_node);
void client_disconnect(struct distr_node *client_node);

int check_auth(struct distr_node *client_node);

int write_node(struct distr_node *client_node, const char *s);
int write_node_end(struct distr_node *client_node, const char *s);
void write_node_end_cb(struct bufferevent *bev, void *arg);

void log_recv(struct distr_node *client_node, const char *s);
void log_send(struct distr_node *client_node, const char *s);

#endif
