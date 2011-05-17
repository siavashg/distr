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
#include <err.h>

#include <event.h>

#include "server.h"

#define SERVER_PORT 9119

int debug = 1;

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

void register_signals() {
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGPIPE, signal_handler);
}

int main(int argc, char **argv) {
	int opt;
	int daemon = 0;
	int port = SERVER_PORT;
	int server_fd;
	pid_t pid, sid;

	/* Launch options */
	while ((opt = getopt(argc, argv, "dvp:")) != -1) {
		switch (opt) {
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

	/* Signal handling */
	register_signals();

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

	/* INIT client nodes */
	TAILQ_INIT(&client_nodes);

	server_fd = server_init(port);
	server_shutdown(server_fd);

	printf("Shutting down server.\n");

	return 0;
}
