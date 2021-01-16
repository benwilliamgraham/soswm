#ifndef SERVER_H
#define SERVER_H

/* Start server */
void server_init();

/* End server */
void server_quit();

/* Host server on separate thread */
void *server_host(void *);

#endif /* !SERVER_H */
