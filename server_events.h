#ifndef SERVER_EVENTS_H
#define SERVER_EVENTS_H

void server_ev_accept(int server_fd, short ev, void *arg);
void server_ev_read(struct bufferevent *bev, void *arg);
void server_ev_write(struct bufferevent *bev, void *arg);
void server_ev_error(struct bufferevent *bev, short what, void *arg);

#endif
