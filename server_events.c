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
#include "protocol.h"
#include "server.h"
#include "server_events.h"


/**
 * libevent events
 */

/**
 * Accept incoming connections event
 */
void server_ev_accept(int server_fd, short ev, void *arg) {
	int client_fd;
	struct distr_node *client_node;
	socklen_t client_len = sizeof(client_node->addr);

	/* Setup client fd */
	client_node = calloc(1, sizeof(*client_node));
	if (client_node == NULL)
		err(1, "alloc failed");

	client_fd = accept(server_fd, (struct sockaddr *)&client_node->addr, &client_len);
	
	if (client_fd < 0) {
		warn("accept failed");
		return;
	}

	if (setnonblock(client_fd) < 0)
		warn("setnonblock on client failed");

	client_node->fd = client_fd;

	/* Setup buffer events for client events */
	client_node->bufev = bufferevent_new(client_node->fd, 
				server_ev_read,
				server_ev_write, 
				server_ev_error, 
				client_node);
	bufferevent_enable(client_node->bufev, EV_READ);

	/* Make sure client connection is initialized */
	client_connect(client_node);
}

/**
 * libevent read callback
 */
void server_ev_read(struct bufferevent *bev, void *arg) {
	struct distr_node *client_node = (struct distr_node*)arg;
	const char *cmd;

	cmd = evbuffer_readline(bev->input);

	if (cmd == NULL) {
		return;
	}
	
	log_recv(client_node, cmd);
	pdstr_parse_cmd(client_node, cmd);
}

/**
 * libevent write callback
 * When write buffer reaches 0
 */
void server_ev_write(struct bufferevent *bev, void *arg) {
	//struct distr_node *client_node = (struct distr_node*)arg;
}

void server_ev_error(struct bufferevent *bev, short what, void *arg) {
	if (what & (EVBUFFER_EOF | EVBUFFER_ERROR)) {
		struct distr_node *client_node = (struct distr_node*)arg;
		client_disconnect(client_node);
	} else {
		printf("ERROR: %d\n", what);
	}
}


