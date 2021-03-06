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
#include "server_events.h"
#include "protocol.h"
#include "server.h"

/* Silently capped at 128, see man listen */
#define LISTEN_BACKLOG 120

int setnonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL);

	if (flags < 0)
		return flags;

	flags |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}

int server_init(int port) {
	unsigned int server_fd;
	struct sockaddr_in server_addr;
	struct event ev_server_accept;
	int reuseaddr_on = 1;

	/* Init libevent */
	event_init();

	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (server_fd < 0)
		err(1, "listen failed");

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr *)&server_addr,
		sizeof(server_addr)) < 0)
		err(1, "bind failed");

	if (listen(server_fd, LISTEN_BACKLOG) < 0)
		err(1, "listen failed");
	
	/* Set SO_REUSEADDR */
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,
		sizeof(reuseaddr_on));
	
	/* Set nonblocking */
	if (setnonblock(server_fd) < 0)
		err(1, "setnonblock failed");

	event_set(&ev_server_accept, server_fd, EV_READ|EV_PERSIST, server_ev_accept, NULL);
	event_add(&ev_server_accept, NULL);

	/* Dispatch to libevent */
	event_dispatch();

	return server_fd;
}

int server_shutdown(int server_fd) {
	shutdown(server_fd, SHUT_RDWR);
	return close(server_fd);
}

void client_connect(struct distr_node *client_node) {
	TAILQ_INSERT_TAIL(&client_nodes, client_node, entries);
	deprintf ("Connected (%s) (fd: %d)\n", inet_ntoa(client_node->addr.sin_addr), client_node->fd);
}

void client_disconnect(struct distr_node *client_node) {
	if (client_node->fd > 0) {
		deprintf ("Disconnected (%s) (fd: %d)\n", inet_ntoa(client_node->addr.sin_addr), client_node->fd);
		shutdown(client_node->fd, SHUT_RDWR);
		bufferevent_free(client_node->bufev);
		close(client_node->fd);
		
		TAILQ_REMOVE(&client_nodes, client_node, entries);

		if (client_node)
			free(client_node);
		
	} else {
		deprintf ("Already disconnected (%s)\n", inet_ntoa(client_node->addr.sin_addr));
		if (client_node)
			free(client_node);
	}
}

int write_node(struct distr_node *client_node, const char *s) {
	struct evbuffer *write_buffer;
	int l;

	write_buffer = evbuffer_new();
	evbuffer_add_printf(write_buffer, "%s", s);
	l = bufferevent_write_buffer(client_node->bufev, write_buffer);
	evbuffer_free(write_buffer);

	if (l == 0)
		log_send(client_node, s);
	else
		deprintf("Error writing to buffer\n");

	return l;
}

/* Disconnect user after write */
int write_node_end(struct distr_node *client_node, const char *s) {
	bufferevent_free(client_node->bufev);
	client_node->bufev = bufferevent_new(client_node->fd, 
				server_ev_read,
				write_node_end_cb, 
				server_ev_error, 
				client_node);
	bufferevent_enable(client_node->bufev, EV_READ);
	return write_node(client_node, s);
}

void write_node_end_cb(struct bufferevent *bev, void *arg) {
	struct distr_node *client_node = (struct distr_node*)arg;
	client_disconnect(client_node);
}

int check_auth(struct distr_node *client_node) {
	if (strlen(client_node->username) <= 0) {
		write_node(client_node, "-ERR User not authenticated\n");
		return 0;
	}
	return 1;
}

void log_recv(struct distr_node *client_node, const char *s) {
	deprintf ("<< ");
	if (strlen(client_node->username) > 0) 
		deprintf("(%s) (%d) ", client_node->username, client_node->fd);
	else 
		deprintf("(%s) (%d) ", inet_ntoa(client_node->addr.sin_addr), client_node->fd);
	deprintf ("%s\n",s);
}

void log_send(struct distr_node *client_node, const char *s) {
	deprintf (">> ");
	if (strlen(client_node->username) > 0) 
		deprintf("(%s) (%d) ", client_node->username, client_node->fd);
	else 
		deprintf("(%s) (%d) ", inet_ntoa(client_node->addr.sin_addr), client_node->fd);
	deprintf ("%s",s);
}
