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
#include "client.h"

struct distr_node *server_connect(char *hostname, unsigned int port) {
	struct distr_node *server_node;
	struct sockaddr_in server_addr;
	int socket_fd;

	server_node = calloc(1, sizeof(*server_node));
	if (server_node == NULL)
		err(1, "alloc failed");

	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, hostname, &server_addr.sin_addr) <= 0) {
		deprintf("Invalid address-family or IP-adress for connect\n");
		err(1, "inet_pton failed");
	}

	socket_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_fd < 0) {
		err(1, "socket failed");
	}

	return server_node;
}
