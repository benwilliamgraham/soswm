#ifndef SERVER_H
#define SERVER_H

/* Start server */
void server_init();

/* End server */
void server_quit();

/* Execute next pending commmand */
void server_exec_commmand();

#endif /* !SERVER_H */
