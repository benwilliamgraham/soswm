#ifndef SERVER_H
#define SERVER_H

/* The fd that new connections will appear on */
extern int connection_socket;

/* Start server */
void server_init();

/* End server */
void server_quit();

/* Handle incoming connection */
void server_handler();

#endif /* !SERVER_H */
