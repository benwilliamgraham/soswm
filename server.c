#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "server.h"

#include "communication.h"
#include "wm.h"

char usage[] = "usage: sosc [--help] <action> <actor> [argument]\n"
               "commmands: \n"
               "sosc push stack\n"
               "sosc pop <window | stack>\n"
               "sosc swap <window | stack> <0...inf>\n"
               "sosc roll <window | stack> <top | bottom>\n"
               "sosc move window <0...inf>\n"
               "sosc set gap <0...inf>\n"
               "sosc split screen <WxH+X+Y> ...\n"
               "sosc logout wm\n"
               "sosc --help\n";

int connection_socket;

void server_init() {
  // create socket
  if ((connection_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0)) == -1) {
    fprintf(stderr, "soswm: Could not initialize socket\n");
    exit(1);
  }

  // create address
  struct sockaddr_un name;
  memset(&name, 0, sizeof(name));
  name.sun_family = AF_UNIX;
  const char *socket_name = "/tmp/soswm.socket";
  strncpy(name.sun_path, socket_name, sizeof(name.sun_path) - 1);
  remove(socket_name);

  // bind socket
  if (bind(connection_socket, (const struct sockaddr *)&name, sizeof(name)) ==
      -1) {
    fprintf(stderr, "soswm: Could not bind socket\n");
    exit(1);
  }

  // listen for connections
  if (listen(connection_socket, REQ_BUFFER_SIZE) == -1) {
    fprintf(stderr, "soswm: Could not bind socket\n");
    exit(1);
  }
}

void server_quit() { close(connection_socket); }

/* Command structure
 *
 * Given that a command is in the form <action> <actor> [argument] ..., create a
 * tree of all possible commands and their resulting handlers.
 *
 * Each action (stored as a NULL-terminated list) has a NULL-terminated list of
 * possible actors, as well as an argument parser, which is NULL when there is
 * no argument.
 */
typedef struct Actor Actor;
typedef struct {
  char *action;
  char *usage;
  struct Actor {
    char *actor;
    void (*handler)();
  } * actor_options;
  void (*arg_parser)();
} Command;

/* Client communication */
int data_socket;
char request[REQ_BUFFER_SIZE];
char reply[REP_BUFFER_SIZE];

#define sock_read(socket, dest) read(socket, dest, sizeof(dest) - 1)
#define sock_writef(socket, dest, ...)                                         \
  write(socket, dest, snprintf(dest, sizeof(dest), __VA_ARGS__) + 1)

/* Argument parsers */
void uint_parser(void (*handler)()) {
  sock_read(data_socket, request);
  int res;
  if (!strcmp("0", request)) {
    res = 0;
  } else if (*request == '-' || !(res = strtoul(request, NULL, 0))) {
    sock_writef(data_socket, reply,
                "Invalid argument: `%s`\nExpected unsigned integer\n", request);
  }
  handler(res);
}

void roll_direction_parser(void (*handler)()) {
  sock_read(data_socket, request);
  if (!strcmp("top", request)) {
    handler(ROLL_TOP);
  } else if (!strcmp("bottom", request)) {
    handler(ROLL_BOTTOM);
  } else {
    sock_writef(data_socket, reply,
                "Invalid argument: `%s`\nExpected `top` or `bottom`\n",
                request);
  }
}

void splits_parser(void (*handler)()) {
  Splits splits = {
      .splits = NULL,
      .num_splits = 0,
  };
  for (;;) {
    sock_read(data_socket, request);
    if (request[0] == '\0') {
      break;
    }
    Split split;
    const unsigned int expected = 4;
    if (sscanf(request, "%ux%u+%d+%d", &split.width, &split.height, &split.x,
               &split.y) != expected) {
      if (splits.splits) {
        free(splits.splits);
      }
      sock_writef(data_socket, reply,
                  "Invalid argument: `%s`\nExpected split in form `WxH+x+y`\n",
                  request);
      return;
    }
    splits.splits =
        realloc(splits.splits, sizeof(Split) * (splits.num_splits + 1));
    splits.splits[splits.num_splits++] = split;
  }
  if (!splits.splits) {
    sock_writef(data_socket, reply,
                "One or more splits must be specified in form `WxH+x+y`\n");
    return;
  }
  handler(splits);
}

Command commands[] = {
    {.usage = "sosc push stack",
     .action = "push",
     .actor_options =
         (Actor[]){{.actor = "stack", .handler = push_stack}, {NULL}},
     .arg_parser = NULL},

    {.usage = "sosc pop <window | stack>",
     .action = "pop",
     .actor_options = (Actor[]){{.actor = "window", .handler = pop_window},
                                {.actor = "stack", .handler = pop_stack},
                                {NULL}},
     .arg_parser = NULL},

    {.usage = "sosc swap <window | stack> <0...inf>",
     .action = "swap",
     .actor_options = (Actor[]){{.actor = "window", .handler = swap_window},
                                {.actor = "stack", .handler = swap_stack},
                                {NULL}},
     .arg_parser = uint_parser},

    {.usage = "sosc roll <window | stack> <top | bottom>",
     .action = "roll",
     .actor_options = (Actor[]){{.actor = "window", .handler = roll_window},
                                {.actor = "stack", .handler = roll_stack},
                                {NULL}},
     .arg_parser = roll_direction_parser},

    {.usage = "sosc move window <0...inf>",
     .action = "move",
     .actor_options =
         (Actor[]){{.actor = "window", .handler = move_window}, {NULL}},
     .arg_parser = uint_parser},

    /* sosc set gap <0...inf> */
    {.usage = "sosc set gap <0...inf>",
     .action = "set",
     .actor_options = (Actor[]){{.actor = "gap", .handler = set_gap}, {NULL}},
     .arg_parser = uint_parser},

    /* sosc split screen <"WxH+X+Y ..."> */
    {.usage = "sosc split screen <WxH+X+Y> ...",
     .action = "split",
     .actor_options =
         (Actor[]){{.actor = "screen", .handler = split_screen}, {NULL}},
     .arg_parser = splits_parser},

    /* sosc logout wm */
    {.usage = "sosc logout wm",
     .action = "logout",
     .actor_options = (Actor[]){{.actor = "wm", .handler = logout_wm}, {NULL}},
     .arg_parser = NULL},
    {NULL},
};

void *server_host(__attribute__((unused)) void *arg) {
  // make sure buffers are null-terminated
  request[sizeof(request) - 1] = 0;
  reply[sizeof(reply) - 1] = 0;

  for (;;) {
    // accept a new connection
    if ((data_socket = accept(connection_socket, NULL, NULL)) == -1) {
      fprintf(stderr, "soswm: Could not accept connection\n");
      exit(1);
    }

    // get lock on wm
    pthread_mutex_lock(&wm_lock);

    // recieve and parse data from client
    sock_read(data_socket, request);

    if (!strcmp("--help", request)) {
      sock_writef(data_socket, reply, "%s\n", usage);
      goto clean_up;
    }

    for (Command *cmd = commands;; cmd++) {
      // if the end of the list is reached, return an error
      if (!cmd->action) {
        sock_writef(data_socket, reply, "Invalid action: `%s`\nExpected: %s\n",
                    request, usage);
        goto clean_up;
      }

      // if the command matches, check actor + arg
      if (!strcmp(cmd->action, request)) {
        sock_read(data_socket, request);

        for (Actor *actor = cmd->actor_options;; actor++) {
          // if the end of the list is reached, return and error
          if (!actor->actor) {
            sock_writef(data_socket, reply,
                        "Invalid actor: `%s`\nExpected: %s\n", request,
                        cmd->usage);
            goto clean_up;
          }

          // if actor matches, check arg
          if (!strcmp(actor->actor, request)) {
            if (cmd->arg_parser) {
              cmd->arg_parser(actor->handler);
            } else {
              actor->handler();
            }
            goto clean_up;
          }
        }
      }
    }

  clean_up:
    // make sure everything is written before shutting down
    pthread_mutex_unlock(&wm_lock);
    shutdown(data_socket, SHUT_WR);
    sock_read(data_socket, request);
    close(data_socket);
  }
  return NULL;
}
