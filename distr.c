#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>

#include <sys/time.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
//#include <errrno.h>
#include <err.h>

#include <event.h>

#include "client.h"

#define SERVER_PORT 9119
int debug = 0;

int dprintf(const char *format, ...) {
	va_list args;
	int l = 0;

	va_start(args, format);

	if(debug)
		l = vprintf(format, args);

	va_end(args);
	return l;
}

void signal_handler(int sig) {
	dprintf("Received signal: %d\n", sig);
	switch (sig) {
		case SIGTERM:
		case SIGHUP:
		case SIGINT:
			event_loopexit(NULL);
			break;
		case SIGPIPE:
			dprintf("Received SIGPIPE\n");
			break;
		default:
			syslog(LOG_WARNING, "Uhandled signal %d", sig);
			break;
	}
}

int main(int argc, char **argv) {
	int listen_fd, ch;
	int daemon = 0;
	int port = SERVER_PORT;
	int reuseaddr_on;

	struct sockaddr_in listen_addr;
	struct event ev_accept;

	pid_t pid, sid;

	/* Signal handling */
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGPIPE, signal_handler);


	/* Launch options */
	while ((ch = getopt(argc, argv, "dvp:")) != -1) {
		switch (ch) {
		case 'd':
			daemon = 1;
			break;
		case 'v':
			debug = 1;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		default:
			printf("Usage: %s [-d] [-v] [-p port]\n", argv[0]);
			exit(EXIT_FAILURE);
			break;

		}
	
	}

	dprintf ("Starting at port %d\n", port);

	/* Daemonify */
	if (daemon) {
		dprintf ("Starting as daemon\n");
		pid = fork();
		if (pid < 0) {
			exit(EXIT_FAILURE);
		} else if (pid > 0) {
			exit(EXIT_SUCCESS);
		}

		umask(0);
		sid = setsid();
		if (sid < 0) {
			exit(EXIT_FAILURE);
		}
	}
	
	TAILQ_INIT(&clients);

	/* Init libevent */
	event_init();

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
		err(1, "listen failed");

	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(port);

	if (bind(listen_fd, (struct sockaddr *)&listen_addr,
		sizeof(listen_addr)) < 0)
		err(1, "bind failed");

	if (listen(listen_fd, 5) < 0)
		err(1, "listen failed");

	reuseaddr_on = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on,
		sizeof(reuseaddr_on));

	if (setnonblock(listen_fd) < 0)
		err(1, "setnonblock failed");

	event_set(&ev_accept, listen_fd, EV_READ|EV_PERSIST, bufev_on_accept, NULL);
	event_add(&ev_accept, NULL);

	event_dispatch();

	shutdown(listen_fd, SHUT_RDWR);
	close(listen_fd);
	printf("Quiting.\n");
	return 0;
}
