#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "server.h"


int pdstr_parse_cmd(struct client_node *client_node, const char *cmd);

int pdstr_greet(struct client_node *client_node);

int pdstr_auth(struct client_node *client_node, const char *cmd);
int pdstr_exit(struct client_node *client_node, const char *cmd);
int pdstr_hello(struct client_node *client_node, const char *cmd);
int pdstr_ping(struct client_node *client_node, const char *cmd);
int pdstr_who(struct client_node *client_node, const char *cmd);
int pdstr_msg(struct client_node *client_node, const char *cmd);
int pdstr_privmsg(struct client_node *client_node, const char *cmd);
int pdstr_http(struct client_node *client_node, const char *cmd);
int pdstr_unknown(struct client_node *client_node, const char *cmd);

#endif
