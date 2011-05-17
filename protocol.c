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

int pdstr_parse_cmd(struct client_node *client_node, const char *cmd) {

	if (strncasecmp(cmd, "AUTH", 4) == 0)
		pdstr_auth(client_node, cmd);
	else if (strncasecmp(cmd, "EXIT", 4) == 0)
		pdstr_exit(client_node, cmd);
	else if (strncasecmp(cmd, "HELLO", 5) == 0)
		pdstr_hello(client_node, cmd);
	else if (strncasecmp(cmd, "PING", 4) == 0)
		pdstr_ping(client_node, cmd);
	else if (strncasecmp(cmd, "WHO", 3) == 0)
		pdstr_who(client_node, cmd);
	else if (strncasecmp(cmd, "MSG", 3) == 0)
		pdstr_msg(client_node, cmd);
	else if (strncasecmp(cmd, "GET", 3) == 0)
		pdstr_http(client_node, cmd);
	else
		pdstr_unknown(client_node, cmd);

	return 0;
}

/*
 * greet
 */
int pdstr_greet(struct client_node *client_node) {
	if (check_auth(client_node)) {
		char *msg;
		asprintf(&msg, "WELCOME %s\n", client_node->username);
		write_node(client_node, msg);
		free(msg);
		return 1;
	}
	return 0;
}


/*
 * AUTH
 */
int pdstr_auth(struct client_node *client_node, const char *cmd) {
	char username[USERNAME_MAX_LENGTH];
	unsigned int port;

	/* TODO: 128 should be USERNAME_MAX_LENGTH */
	if (sscanf(&cmd[5], "%128s %d", username, &port) == 2) {
		struct client_node *cli;
		
		/* Check if username is already taken */
		TAILQ_FOREACH(cli, &client_nodes, entries) {
			/* Skip self */
			if (cli->fd == client_node->fd)
				continue;

			if (strcasecmp(cli->username, username) == 0) {
				write_node(client_node, "-ERR Auth failed. Username is already in use.\n");
				return -1;
			}
		}

		/* All good */
		strcpy(client_node->username, username);
		client_node->port = port;
		write_node(client_node, "+OK\n");

		return pdstr_greet(client_node);
	} else {
		write_node(client_node, "-ERR Auth failed. Invalid arguments.\n");
		return 0;
	}
}

/*
 * EXIT
 */
int pdstr_exit(struct client_node *client_node, const char *cmd) {
	int r = write_node_end(client_node, "BYE\n");
	return r;
}

/*
 * HELLO
 */
int pdstr_hello(struct client_node *client_node, const char *cmd) {
	if (check_auth(client_node)) {
		char msg[128];
		snprintf(msg, 128, "HELLO %s\n", client_node->username);
		return write_node(client_node, msg);
	} 
	return 0;
}

/*
 * PING
 */
int pdstr_ping(struct client_node *client_node, const char *cmd) {
	return write_node(client_node, "PONG\n");
}

/*
 * GET
 */
int pdstr_http(struct client_node *client_node, const char *cmd) {
	char html[] = "<html><body>Hej!</body></html>\n";
	write_node(client_node, "HTTP/1.1 200\n");
	write_node(client_node, "Content-Length: 112\n");
	write_node_end(client_node, html);
	return 1;
}

/*
 * WHO
 */
int pdstr_who(struct client_node *client_node, const char *cmd) {
	if(check_auth(client_node)) {
		struct client_node *cli;
		TAILQ_FOREACH(cli, &client_nodes, entries) {
			char msg[1024];
			snprintf(msg, 1024, "USER %s %s %d\n", 
				cli->username,
				inet_ntoa(cli->addr.sin_addr),
				cli->port);
			write_node(client_node, msg);
		}
		return 1;
	}
	return 0;
}

/*
 * MSG
 */
int pdstr_msg(struct client_node *client_node, const char *cmd) {
	if(check_auth(client_node)) {
		struct client_node *cli;
		char msg[1024];

		snprintf(msg, 1024, "MSG %s %s\n", client_node->username, &cmd[4]);

		TAILQ_FOREACH(cli, &client_nodes, entries) {
			/* Skip self */
			if(cli->fd == client_node->fd)
				continue;
			write_node(cli, msg);
		}

		/**
		 * Send to the sender last 
		 * Avoid disconnecting and thus freeing "client_node" if the sender
		 * disconnects
		 */
		write_node(client_node, msg);
		return 1;
	}
	return 0;
}

/*
 * UNKNOWN
 */
int pdstr_unknown(struct client_node *client_node, const char *cmd) {
	return write_node(client_node, "-ERR Unknown command\n");
}

