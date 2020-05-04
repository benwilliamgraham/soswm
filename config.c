#include <X11/Xutil.h>

typedef union {
  unsigned int i;
  char **s;
} Arg;

/* Window manipulation functions */
void window_new(Arg);
void window_roll_l(Arg);
void window_roll_r(Arg);
void window_swap(Arg);
void window_push(Arg);
void window_quit(Arg);

/* Workspace manipulation functions */
void workspace_new(Arg);
void workspace_fullscreen(Arg);
void workspace_shrink(Arg);
void workspace_grow(Arg);
void workspace_roll_l(Arg);
void workspace_roll_r(Arg);
void workspace_swap(Arg);
void workspace_quit(Arg);

/* Window manager manipulation functions */
void wm_restart(Arg);
void wm_logout(Arg);

/* Startup programs */
char **startup_programs[] = {(char *[]){
    "feh", "--bg-scale", "/home/benwilliamgraham/Pictures/Mountain.png", NULL}};

/* Keybindings */
unsigned int mod = Mod4Mask;
struct {
  char *key;
  unsigned int mask;
  void (*handler)(Arg);
  Arg arg;
  unsigned int keycode;
} keybinds[] = {
    /* Window manipulation */
    {"j", 0, window_roll_l},
    {"k", 0, window_roll_r},
    {"1", 0, window_swap, {.i = 1}},
    {"2", 0, window_swap, {.i = 2}},
    {"3", 0, window_swap, {.i = 3}},
    {"4", 0, window_swap, {.i = 4}},
    {"5", 0, window_swap, {.i = 5}},
    {"6", 0, window_swap, {.i = 6}},
    {"7", 0, window_swap, {.i = 7}},
    {"8", 0, window_swap, {.i = 8}},
    {"9", 0, window_swap, {.i = 9}},
    {"1", ControlMask, window_push, {.i = 1}},
    {"2", ControlMask, window_push, {.i = 2}},
    {"3", ControlMask, window_push, {.i = 3}},
    {"4", ControlMask, window_push, {.i = 4}},
    {"5", ControlMask, window_push, {.i = 5}},
    {"6", ControlMask, window_push, {.i = 6}},
    {"7", ControlMask, window_push, {.i = 7}},
    {"8", ControlMask, window_push, {.i = 8}},
    {"9", ControlMask, window_push, {.i = 9}},
    {"q", 0, window_quit},

    /* Launchers */
    {"space", 0, window_new, {.s = (char *[]){"rofi", "-show", "drun", NULL}}},
    {"t", 0, window_new, {.s = (char *[]){"kitty", NULL}}},
    {"e", 0, window_new, {.s = (char *[]){"kitty", "nvim", NULL}}},

    /* Workspace manipulation */
    {"n", ShiftMask, workspace_new},
    {"f", ShiftMask, workspace_fullscreen},
    {"h", ShiftMask, workspace_shrink},
    {"l", ShiftMask, workspace_grow},
    {"j", ShiftMask, workspace_roll_l},
    {"k", ShiftMask, workspace_roll_r},
    {"1", ShiftMask, workspace_swap, {.i = 1}},
    {"2", ShiftMask, workspace_swap, {.i = 2}},
    {"3", ShiftMask, workspace_swap, {.i = 3}},
    {"4", ShiftMask, workspace_swap, {.i = 4}},
    {"5", ShiftMask, workspace_swap, {.i = 5}},
    {"6", ShiftMask, workspace_swap, {.i = 6}},
    {"7", ShiftMask, workspace_swap, {.i = 7}},
    {"8", ShiftMask, workspace_swap, {.i = 8}},
    {"9", ShiftMask, workspace_swap, {.i = 9}},
    {"q", ShiftMask, workspace_quit},

    /* Window manager manipulation */
    {"r", Mod1Mask, wm_restart},
    {"l", Mod1Mask, wm_logout}};
const unsigned int num_keybinds = sizeof(keybinds) / sizeof(*keybinds);

/* Graphical customization */
const int gaps = 8;
const float def_ratio = 1.f;
