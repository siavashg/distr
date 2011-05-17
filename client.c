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
//#include <errrno.h>
#include <err.h>

#include <event.h>

#include "queue.h"
#include "distr.h"
#include "client.h"

void client_disconnect(struct client *client) {

	if (client->fd > 0) {
		dprintf ("Disconnected (%s) (fd: %d)\n", inet_ntoa(client->client_addr.sin_addr), client->fd);
		shutdown(client->fd, SHUT_RDWR);
		bufferevent_free(client->buf_ev);
		close(client->fd);
		
		TAILQ_REMOVE(&clients, client, entries);

		if (client)
			free(client);
		
	} else {
		dprintf ("Already disconnected (%s)\n", inet_ntoa(client->client_addr.sin_addr));
		if (client)
			free(client);
	}
}

void log_recv(struct client *client, const char *s) {
	dprintf (">> ");
	if (strlen(client->username) > 0) 
		dprintf("(%s)", client->username);
	else 
		dprintf("(%s)", inet_ntoa(client->client_addr.sin_addr));
	dprintf ("%s\n",s);
}

void log_send(struct client *client, const char *s) {
	dprintf ("<< ");
	if (strlen(client->username) > 0) 
		dprintf("(%s)", client->username);
	else 
		dprintf("(%s)", inet_ntoa(client->client_addr.sin_addr));
	dprintf ("%s\n",s);
}

void client_connect(struct client *client) {
	struct evbuffer *evbuffer;

	evbuffer = evbuffer_new();
	TAILQ_INSERT_TAIL(&clients, client, entries);

	evbuffer_add_printf(evbuffer, "WELCOME\n");
	evbuffer_write(evbuffer, client->fd);

	evbuffer_free(evbuffer);

	dprintf ("Connected (%s) (fd: %d)\n", inet_ntoa(client->client_addr.sin_addr), client->fd);
}

//int cli_writef(struct client *client, const char *format, ...) {
int cwrite(struct client *client, const char *s) {
	struct evbuffer *evbuffer;
	int l;

	dprintf("DEBUG: %s\n", s);

	evbuffer = evbuffer_new();
	evbuffer_add_printf(evbuffer, s);
	l = bufferevent_write_buffer(client->buf_ev, evbuffer);
	evbuffer_free(evbuffer);
	log_send(client, s);
	return l;
}

/*
 * libevent read callback
 */
void bufev_on_read(struct bufferevent *bev, void *arg) {
	struct client *client = (struct client*)arg;
	struct client *cli;
	struct evbuffer *evbuffer;

	char *cmd;
	char buf[16384];
	*buf = '\0';

	cmd = evbuffer_readline(bev->input);

	if (cmd == NULL) {
		return;
	}
	
	log_recv(client, cmd);

	evbuffer = evbuffer_new();
	if (strncasecmp(cmd, "INIT", 4) == 0) {
		if (sscanf(&cmd[5], "%s %d", client->username, &client->port) == 2) {
			cwrite(client, "+OK\n");
		} else {
			cwrite(client, "-ERR Init failed\n");
		}
	}

	if (strlen(client->username) <= 0) {
		cwrite(client, "-ERR User not authenticated\n");
	}
	else if (strncasecmp(cmd, "HELLO", 5) == 0) {
		cwrite(client, "HELLO\n");
	} 
	else if (strncasecmp(cmd, "WHO", 3) == 0) {
		TAILQ_FOREACH(cli, &clients, entries) {
			char msg[1024];
			snprintf(msg, 1024, "USER %s %s %d\n", 
				cli->username,
				inet_ntoa(cli->client_addr.sin_addr),
				cli->port);
			cwrite(client, msg);
		}
	}
	else if (strncasecmp(cmd, "MSG", 3) == 0) {
		char msg[1024];
		snprintf(msg, 1024, "MSG %s %s\n", client->username, &cmd[4]);

		TAILQ_FOREACH(cli, &clients, entries) {
			if(cli->fd == client->fd)
				break;
			cwrite(cli, msg);
		}

		/**
		 * Send to the sender last 
		 * Avoid disconnecting and thus freeing "client" if the sender
		 * disconnects
		 */
		cwrite(client, msg);

	}
	else if (strncasecmp(cmd, "EXIT", 4) == 0) {
		cwrite(client, "BYE\n");
		shutdown(client->fd, SHUT_RD);
	}


	bufferevent_write_buffer(bev, evbuffer);
	evbuffer_free(evbuffer);

	if (cmd)
		free(cmd);
}

/**
 * libevent write callback
 * When write buffer reaches 0
 */
void bufev_on_write(struct bufferevent *bev, void *arg) {
}

void bufev_on_error(struct bufferevent *bev, short what, void *arg) {
	if (what & (EVBUFFER_EOF | EVBUFFER_ERROR)) {
		struct client *client = (struct client*)arg;
		client_disconnect(client);
	} else {
		printf("ERROR: %d\n", what);
	}
}


void bufev_on_accept(int fd, short ev, void *arg) {
	int client_fd;
	struct client *client;

	socklen_t client_len = sizeof(client->client_addr);

	client = calloc(1, sizeof(*client));
	if (client == NULL)
		err(1, "alloc failed");

	client_fd = accept(fd, (struct sockaddr *)&client->client_addr, &client_len);
	
	if (client_fd < 0) {
		warn("accept failed");
		return;
	}

	if (setnonblock(client_fd) < 0)
		warn("setnonblock on client failed");

	client->fd = client_fd;

	client->buf_ev = bufferevent_new(client_fd, bufev_on_read, 
		bufev_on_write, bufev_on_error, client);
	bufferevent_enable(client->buf_ev, EV_READ);
	
	client_connect(client);


}


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

