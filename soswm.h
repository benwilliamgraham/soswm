#ifndef SOSWM_H
#define SOSWM_H

/* Callback function argument struct and initializer macros */
typedef union {
  unsigned int i;
  char **p;
} Arg;

#define INT_ARG(i_val)                                                         \
  (Arg) { .i = (i_val) }
#define PROG_ARG(...)                                                          \
  (Arg) {                                                                      \
    .p = (char *[]) { __VA_ARGS__, NULL }                                      \
  }

/* Window manipulation functions */
void window_push(Arg);
void window_pop(Arg);
void window_swap(Arg);
void window_roll_l(Arg);
void window_roll_r(Arg);
void window_move(Arg);

/* Workspace manipulation functions */
void workspace_push(Arg);
void workspace_pop(Arg);
void workspace_swap(Arg);
void workspace_roll_l(Arg);
void workspace_roll_r(Arg);
void workspace_fullscreen(Arg);
void workspace_shrink(Arg);
void workspace_grow(Arg);

/* Window manager manipulation functions */
void mon_swap(Arg);
void mon_roll_l(Arg);
void mon_roll_r(Arg);
void wm_refresh(Arg);
void wm_replace(Arg);
void wm_logout(Arg);

#endif /* !SOSWM_H */
