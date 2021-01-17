#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "communication.h"

int main(int argc, char *argv[]) {
  // create socket
  int data_socket;
  if ((data_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1) {
    fprintf(stderr, "sosc: Could not create socket\n");
    exit(1);
  }

  // create address
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, "/tmp/soswm.socket", sizeof(addr.sun_path) - 1);

  // connect to soswm
  if (connect(data_socket, (const struct sockaddr *)&addr, sizeof(addr)) ==
      -1) {
    fprintf(stderr, "sosc: Could not connect to server\n");
    exit(1);
  }

  // send arguments
  for (char **arg = argv + 1; arg < argv + argc; arg++) {
    if (write(data_socket, *arg, strlen(*arg) + 1) == -1) {
      fprintf(stderr, "sosc: Could not send arguments\n");
      exit(1);
    }
  }
  if (write(data_socket, "\0", 1) == -1) {
    fprintf(stderr, "sosc: Could not send end specifier\n");
    exit(1);
  }

  // output reply from soswm, if any
  char reply[REP_BUFFER_SIZE];
  if (read(data_socket, reply, sizeof(reply)) > 0) {
    reply[sizeof(reply) - 1] = '\0';
    printf("soswm: %s", reply);
  }

  close(data_socket);
}
