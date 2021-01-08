#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "server.h"

#include "communication.h"
#include "soswm.h"

char usage[] = "usage: sosc [--help] <action> <actor> [argument]\n"
               "commmands: \n"
               "  sosc push workspace\n"
               "  sosc pop <window | workspace>\n"
               "  sosc swap <window | workspace | monitor> <n>\n"
               "  sosc roll <window | workspace | monitor> <left | right>\n"
               "  sosc move window <n>\n"
               "  sosc replace wm\n"
               "  sosc logout wm\n"
               "  sosc layout workspace <fullscreen | halved | toggle>\n"
               "  sosc scale workspace <bigger | smaller | reset>\n"
               "  sosc gap <top | bottom | left | right | inner> <n>\n"
               "  sosc --help\n";

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

bool server_has_command() { return false; }

/* Argument parsers */
const char *uint_parser(unsigned int *res, char *lit) {
  if (!strcmp("0", lit)) {
    *res = 0;
  } else if (*lit == '-' || !(*res = strtoul(lit, NULL, 0))) {
    return "unsigned integer";
  }
  return NULL;
}

const char *roll_direction_parser(unsigned int *res, char *lit) {
  if (!strcmp("left", lit)) {
    *res = ROLL_LEFT;
  } else if (!strcmp("right", lit)) {
    *res = ROLL_RIGHT;
  } else {
    return "<left | right>";
  }
  return NULL;
}

const char *layout_mode_parser(unsigned int *res, char *lit) {
  if (!strcmp("fullscreen", lit)) {
    *res = LAYOUT_FULLSCREEN;
  } else if (!strcmp("halved", lit)) {
    *res = LAYOUT_HALVED;
  } else if (!strcmp("toggle", lit)) {
    *res = LAYOUT_TOGGLE;
  } else {
    return "<fullscreen | halved | toggle>";
  }
  return NULL;
}

const char *scale_mode_parser(unsigned int *res, char *lit) {
  if (!strcmp("bigger", lit)) {
    *res = SCALE_BIGGER;
  } else if (!strcmp("smaller", lit)) {
    *res = SCALE_SMALLER;
  } else if (!strcmp("reset", lit)) {
    *res = SCALE_RESET;
  } else {
    return "<bigger | smaller | reset>";
  }
  return NULL;
}

/* Command structure
 *
 * Given that a command is in the form <action> <actor> [argument], create a
 * tree of all possible commands and their resulting handlers.
 *
 * Each action (stored as a NULL-terminated list) has a NULL-terminated list of
 * possible actors, as well as an argument parser, which is NULL when there is
 * no argument.
 */
typedef struct Actor Actor;
typedef struct Command Command;
struct Command {
  char *action;
  char *usage;
  struct Actor {
    char *actor;
    union {
      void (*handler_void)();
      void (*handler_uint)(unsigned int);
    };
  } * actor_options;
  const char *(*arg_parser)(unsigned int *, char *);
} commands[] = {
    {.usage = "sosc push <window | workspace>",
     .action = "push",
     .actor_options =
         (Actor[]){{.actor = "workspace", .handler_void = push_workspace},
                   {NULL}},
     .arg_parser = NULL},

    {.usage = "sosc pop <window | workspace>",
     .action = "pop",
     .actor_options =
         (Actor[]){{.actor = "window", .handler_void = pop_window},
                   {.actor = "workspace", .handler_void = pop_workspace},
                   {NULL}},
     .arg_parser = NULL},

    {.usage = "sosc swap <window | workspace | monitor> <n>",
     .action = "swap",
     .actor_options =
         (Actor[]){{.actor = "window", .handler_uint = swap_window},
                   {.actor = "workspace", .handler_uint = swap_workspace},
                   {.actor = "monitor", .handler_uint = swap_monitor},
                   {NULL}},
     .arg_parser = uint_parser},

    {.usage = "sosc roll <window | workspace | monitor> <left | right>",
     .action = "roll",
     .actor_options =
         (Actor[]){{.actor = "window", .handler_uint = roll_window},
                   {.actor = "workspace", .handler_uint = roll_window},
                   {.actor = "monitor", .handler_uint = roll_window},
                   {NULL}},
     .arg_parser = roll_direction_parser},

    {.usage = "sosc move window <n>",
     .action = "move",
     .actor_options =
         (Actor[]){{.actor = "window", .handler_uint = move_window}, {NULL}},
     .arg_parser = uint_parser},

    /* sosc replace wm */
    {.action = "replace",
     .actor_options =
         (Actor[]){{.actor = "wm", .handler_void = replace_wm}, {NULL}},
     .arg_parser = NULL},

    /* sosc logout wm */
    {.action = "logout",
     .actor_options =
         (Actor[]){{.actor = "wm", .handler_void = logout_wm}, {NULL}},
     .arg_parser = NULL},

    /* sosc layout workspace <fullscreen | halved | toggle> */
    {.action = "layout",
     .actor_options =
         (Actor[]){{.actor = "workspace", .handler_uint = layout_workspace},
                   {NULL}},
     .arg_parser = layout_mode_parser},

    /* sosc scale workspace <bigger | smaller | reset> */
    {.action = "scale",
     .actor_options =
         (Actor[]){{.actor = "workspace", .handler_uint = scale_workspace},
                   {NULL}},
     .arg_parser = scale_mode_parser},

    /* sosc gap <top | bottom | left | right | inner> <n> */
    {.action = "gap",
     .actor_options = (Actor[]){{.actor = "top", .handler_uint = gap_top},
                                {.actor = "bottom", .handler_uint = gap_bottom},
                                {.actor = "left", .handler_uint = gap_left},
                                {.actor = "right", .handler_uint = gap_right},
                                {.actor = "inner", .handler_uint = gap_inner},
                                {NULL}},
     .arg_parser = uint_parser},
    {NULL},
};

/* Socket read-write macros to eliminate repeated code */
#define sock_read(socket, dest) read(socket, dest, sizeof(dest) - 1)
#define sock_writef(socket, dest, ...) write(socket, dest, snprintf(dest, sizeof(dest), __VA_ARGS__) + 1)

void server_exec_commmands() {
  // accept a new connection
  int data_socket;
  if ((data_socket = accept(connection_socket, NULL, NULL)) == -1) {
    fprintf(stderr, "soswm: Could not accept connection\n");
    exit(1);
  }

  // create buffers, making sure they are null-terminated
  char request[REQ_BUFFER_SIZE];
  char reply[REP_BUFFER_SIZE];
  request[sizeof(request) - 1] = 0;
  reply[sizeof(reply) - 1] = 0;

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
          sock_writef(data_socket, reply, "Invalid actor: `%s`\nExpected: %s\n",
                       request, cmd->usage);
          goto clean_up;
        }

        // if actor matches, check arg
        if (!strcmp(actor->actor, request)) {
          if (cmd->arg_parser) {
            sock_read(data_socket, request);
            unsigned int arg;
            const char *arg_usage_err = cmd->arg_parser(&arg, request);
            if (arg_usage_err) {
              sock_writef(data_socket, reply,
                                   "Invalid arg: `%s`\nExpected: %s\n", request,
                                   arg_usage_err);
            } else {
              actor->handler_uint(arg);
            }
          } else {
            actor->handler_void();
          }
          goto clean_up;
        }
      }
    }
  }

clean_up:
  // make sure everything is written before shutting down
  shutdown(data_socket, SHUT_WR);
  sock_read(data_socket, request);
  close(data_socket);
}
