#include <X11/Xutil.h>

typedef union {
  unsigned int i;
  char **s;
} Arg;

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
void wm_restart(Arg);
void wm_logout(Arg);

/* Startup programs */
char **programs[] = {(char *[]){
    "feh", "--bg-scale", "/home/benwilliamgraham/Pictures/mountain.jpg", NULL}};

/* Keybindings */
struct KeyBind {
  char *key;
  unsigned int mask;
  void (*handler)(Arg);
  Arg arg;
  unsigned int keycode;
} keybinds[] = {
    /* Window manipulation */
    {"j", Mod4Mask, window_roll_l},
    {"k", Mod4Mask, window_roll_r},
    {"q", Mod4Mask, window_pop},
    {"1", Mod4Mask, window_swap, {.i = 1}},
    {"2", Mod4Mask, window_swap, {.i = 2}},
    {"3", Mod4Mask, window_swap, {.i = 3}},
    {"4", Mod4Mask, window_swap, {.i = 4}},
    {"5", Mod4Mask, window_swap, {.i = 5}},
    {"6", Mod4Mask, window_swap, {.i = 6}},
    {"7", Mod4Mask, window_swap, {.i = 7}},
    {"8", Mod4Mask, window_swap, {.i = 8}},
    {"9", Mod4Mask, window_swap, {.i = 9}},
    {"n", Mod4Mask | ControlMask, window_move, {.i = 0}},
    {"1", Mod4Mask | ControlMask, window_move, {.i = 1}},
    {"2", Mod4Mask | ControlMask, window_move, {.i = 2}},
    {"3", Mod4Mask | ControlMask, window_move, {.i = 3}},
    {"4", Mod4Mask | ControlMask, window_move, {.i = 4}},
    {"5", Mod4Mask | ControlMask, window_move, {.i = 5}},
    {"6", Mod4Mask | ControlMask, window_move, {.i = 6}},
    {"7", Mod4Mask | ControlMask, window_move, {.i = 7}},
    {"8", Mod4Mask | ControlMask, window_move, {.i = 8}},
    {"9", Mod4Mask | ControlMask, window_move, {.i = 9}},

    /* Launchers */
    {"space",
     Mod4Mask,
     window_push,
     {.s = (char *[]){"rofi", "-show", "drun", NULL}}},
    {"t", Mod4Mask, window_push, {.s = (char *[]){"kitty", NULL}}},
    {"e", Mod4Mask, window_push, {.s = (char *[]){"kitty", "nvim", NULL}}},

    /* Workspace manipulation */
    {"n", Mod4Mask | ShiftMask, workspace_push},
    {"q", Mod4Mask | ShiftMask, workspace_pop},
    {"f", Mod4Mask | ShiftMask, workspace_fullscreen},
    {"h", Mod4Mask | ShiftMask, workspace_shrink},
    {"l", Mod4Mask | ShiftMask, workspace_grow},
    {"j", Mod4Mask | ShiftMask, workspace_roll_l},
    {"k", Mod4Mask | ShiftMask, workspace_roll_r},
    {"1", Mod4Mask | ShiftMask, workspace_swap, {.i = 1}},
    {"2", Mod4Mask | ShiftMask, workspace_swap, {.i = 2}},
    {"3", Mod4Mask | ShiftMask, workspace_swap, {.i = 3}},
    {"4", Mod4Mask | ShiftMask, workspace_swap, {.i = 4}},
    {"5", Mod4Mask | ShiftMask, workspace_swap, {.i = 5}},
    {"6", Mod4Mask | ShiftMask, workspace_swap, {.i = 6}},
    {"7", Mod4Mask | ShiftMask, workspace_swap, {.i = 7}},
    {"8", Mod4Mask | ShiftMask, workspace_swap, {.i = 8}},
    {"9", Mod4Mask | ShiftMask, workspace_swap, {.i = 9}},

    /* Window manager manipulation */
    {"j", Mod4Mask | Mod1Mask, mon_roll_l},
    {"k", Mod4Mask | Mod1Mask, mon_roll_r},
    {"1", Mod4Mask | Mod1Mask, mon_swap, {.i = 1}},
    {"2", Mod4Mask | Mod1Mask, mon_swap, {.i = 2}},
    {"3", Mod4Mask | Mod1Mask, mon_swap, {.i = 3}},
    {"4", Mod4Mask | Mod1Mask, mon_swap, {.i = 4}},
    {"5", Mod4Mask | Mod1Mask, mon_swap, {.i = 5}},
    {"6", Mod4Mask | Mod1Mask, mon_swap, {.i = 6}},
    {"7", Mod4Mask | Mod1Mask, mon_swap, {.i = 7}},
    {"8", Mod4Mask | Mod1Mask, mon_swap, {.i = 8}},
    {"9", Mod4Mask | Mod1Mask, mon_swap, {.i = 9}},
    {"r", Mod4Mask | Mod1Mask, wm_restart},
    {"l", Mod4Mask | Mod1Mask, wm_logout}};

/* Graphical customization */
const int gaps = 8;
const float def_ratio = 1.f;
