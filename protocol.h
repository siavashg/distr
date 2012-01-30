#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "server.h"

int pdstr_parse_cmd(struct distr_node *client_node, const char *cmd);

int pdstr_greet(struct distr_node *client_node);

int pdstr_auth(struct distr_node *client_node, const char *cmd);
int pdstr_exit(struct distr_node *client_node, const char *cmd);
int pdstr_hello(struct distr_node *client_node, const char *cmd);
int pdstr_ping(struct distr_node *client_node, const char *cmd);
int pdstr_http(struct distr_node *client_node, const char *cmd);
int pdstr_who(struct distr_node *client_node, const char *cmd);
int pdstr_msg(struct distr_node *client_node, const char *cmd);
int pdstr_connect(struct distr_node *client_node, const char *cmd);
int pdstr_unknown(struct distr_node *client_node, const char *cmd);

#endif
