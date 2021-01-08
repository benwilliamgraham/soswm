#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>

/* Start server */
void server_init();

/* End server */
void server_quit();

/* Return if the server has a pending command. */
bool server_has_command();

/* Execute all pending commmands */
void server_exec_commmands();

#endif /* !SERVER_H */
