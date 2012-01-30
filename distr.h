#ifndef DISTR_H
#define DISTR_H

#define USERNAME_MAX_LENGTH 128

struct distr_node {
	int fd;

	struct bufferevent *bufev;
	struct sockaddr_in addr;
	int port;

	char username[USERNAME_MAX_LENGTH];

	TAILQ_ENTRY(distr_node) entries;
};

int deprintf(const char *format, ...);

#endif
