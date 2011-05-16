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
#include "server.h"
#include "protocol.h"

int _write_node(struct client_node *client_node, const char *s) {
	struct evbuffer *write_buffer;
	int l;

	dprintf("_write_node(%s, %s)\n", client_node->username, s);

	write_buffer = evbuffer_new();

	evbuffer_add_printf(write_buffer, "%s", s);

	l = bufferevent_write_buffer(client_node->bufev, write_buffer);

	evbuffer_free(write_buffer);
	if (l == 0)
		log_send(client_node, s);
	else
		dprintf("Error writing to buffer\n");

	return l;
}

int _check_auth(struct client_node *client_node) {
	if (strlen(client_node->username) <= 0) {
		_write_node(client_node, "-ERR User not authenticated\n");
		return 0;
	}
	return 1;
}

int pdstr_parse_cmd(struct client_node *client_node, const char *cmd) {

	if (strncasecmp(cmd, "INIT", 4) == 0)
		pdstr_init(client_node, cmd);
	else if (strncasecmp(cmd, "HELLO", 5) == 0)
		pdstr_hello(client_node, cmd);
	else if (strncasecmp(cmd, "PING", 4) == 0)
		pdstr_ping(client_node, cmd);
	else if (strncasecmp(cmd, "WHO", 3) == 0)
		pdstr_who(client_node, cmd);
	else if (strncasecmp(cmd, "MSG", 3) == 0)
		pdstr_msg(client_node, cmd);
	else if (strncasecmp(cmd, "EXIT", 4) == 0) {
		pdstr_exit(client_node, cmd);
	}

	return -1;
}


int pdstr_init(struct client_node *client_node, const char *cmd) {
	if (sscanf(&cmd[5], "%s %d", client_node->username, &client_node->port) == 2) {
		/* Verify if username is unique */
		return _write_node(client_node, "+OK\n");
	} else {
		return _write_node(client_node, "-ERR Init failed\n");
	}
	return -1;
}

int pdstr_greet(struct client_node *client_node) {
	return _write_node(client_node, "WELCOME\n");
}

int pdstr_exit(struct client_node *client_node, const char *cmd) {
	int r =_write_node(client_node, "BYE\n");
	shutdown(client_node->fd, SHUT_RD);
	return r;
}

int pdstr_hello(struct client_node *client_node, const char *cmd) {
	if (_check_auth(client_node)) {
		char msg[128];
		snprintf(msg, 128, "HELLO %s\n", client_node->username);
		return _write_node(client_node, msg);
	} 
	return -1;
}

int pdstr_ping(struct client_node *client_node, const char *cmd) {
	return _write_node(client_node, "PONG\n");
}

int pdstr_who(struct client_node *client_node, const char *cmd) {
	if(_check_auth(client_node)) {
		struct client_node *cli;
		TAILQ_FOREACH(cli, &client_nodes, entries) {
			char msg[1024];
			snprintf(msg, 1024, "USER %s %s %d\n", 
				cli->username,
				inet_ntoa(cli->addr.sin_addr),
				cli->port);
			_write_node(client_node, msg);
		}
		return 0;
	}
	return -1;
}


int pdstr_msg(struct client_node *client_node, const char *cmd) {
	if(_check_auth(client_node)) {
		struct client_node *cli;
		char msg[1024];

		snprintf(msg, 1024, "MSG %s %s\n", client_node->username, &cmd[4]);

		TAILQ_FOREACH(cli, &client_nodes, entries) {
			if(cli->fd == client_node->fd)
				continue;
			_write_node(cli, msg);
		}
		/**
		 * Send to the sender last 
		 * Avoid disconnecting and thus freeing "client_node" if the sender
		 * disconnects
		 */
		_write_node(client_node, msg);
		return 0;
	}
	return -1;
}


void log_recv(struct client_node *client_node, const char *s) {
	dprintf (">> ");
	if (strlen(client_node->username) > 0) 
		dprintf("(%s)", client_node->username);
	else 
		dprintf("(%s)", inet_ntoa(client_node->addr.sin_addr));
	dprintf ("%s\n",s);
}

void log_send(struct client_node *client_node, const char *s) {
	dprintf ("<< ");
	if (strlen(client_node->username) > 0) 
		dprintf("(%s)", client_node->username);
	else 
		dprintf("(%s)", inet_ntoa(client_node->addr.sin_addr));
	dprintf ("%s\n",s);
}
