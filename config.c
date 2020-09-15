#include <X11/XF86keysym.h>
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
char **programs[] = {(char *[]){"feh", "--bg-scale",
                                "/usr/share/backgrounds/f32/default/f32.png",
                                NULL},
                     (char *[]){"gnome-flashback", NULL}};

/* Keybindings */
struct KeyBind {
  unsigned int key, mask;
  void (*handler)(Arg);
  Arg arg;
  unsigned int keycode;
} keybinds[] = {
    /* Window manipulation */
    {XK_j, Mod4Mask, window_roll_l},
    {XK_k, Mod4Mask, window_roll_r},
    {XK_q, Mod4Mask, window_pop},
    {XK_1, Mod4Mask, window_swap, {.i = 1}},
    {XK_2, Mod4Mask, window_swap, {.i = 2}},
    {XK_3, Mod4Mask, window_swap, {.i = 3}},
    {XK_4, Mod4Mask, window_swap, {.i = 4}},
    {XK_5, Mod4Mask, window_swap, {.i = 5}},
    {XK_6, Mod4Mask, window_swap, {.i = 6}},
    {XK_7, Mod4Mask, window_swap, {.i = 7}},
    {XK_8, Mod4Mask, window_swap, {.i = 8}},
    {XK_9, Mod4Mask, window_swap, {.i = 9}},
    {XK_n, Mod4Mask | ControlMask, window_move, {.i = 0}},
    {XK_1, Mod4Mask | ControlMask, window_move, {.i = 1}},
    {XK_2, Mod4Mask | ControlMask, window_move, {.i = 2}},
    {XK_3, Mod4Mask | ControlMask, window_move, {.i = 3}},
    {XK_4, Mod4Mask | ControlMask, window_move, {.i = 4}},
    {XK_5, Mod4Mask | ControlMask, window_move, {.i = 5}},
    {XK_6, Mod4Mask | ControlMask, window_move, {.i = 6}},
    {XK_7, Mod4Mask | ControlMask, window_move, {.i = 7}},
    {XK_8, Mod4Mask | ControlMask, window_move, {.i = 8}},
    {XK_9, Mod4Mask | ControlMask, window_move, {.i = 9}},

    /* Workspace manipulation */
    {XK_n, Mod4Mask | ShiftMask, workspace_push},
    {XK_q, Mod4Mask | ShiftMask, workspace_pop},
    {XK_f, Mod4Mask | ShiftMask, workspace_fullscreen},
    {XK_h, Mod4Mask | ShiftMask, workspace_shrink},
    {XK_l, Mod4Mask | ShiftMask, workspace_grow},
    {XK_j, Mod4Mask | ShiftMask, workspace_roll_l},
    {XK_k, Mod4Mask | ShiftMask, workspace_roll_r},
    {XK_1, Mod4Mask | ShiftMask, workspace_swap, {.i = 1}},
    {XK_2, Mod4Mask | ShiftMask, workspace_swap, {.i = 2}},
    {XK_3, Mod4Mask | ShiftMask, workspace_swap, {.i = 3}},
    {XK_4, Mod4Mask | ShiftMask, workspace_swap, {.i = 4}},
    {XK_5, Mod4Mask | ShiftMask, workspace_swap, {.i = 5}},
    {XK_6, Mod4Mask | ShiftMask, workspace_swap, {.i = 6}},
    {XK_7, Mod4Mask | ShiftMask, workspace_swap, {.i = 7}},
    {XK_8, Mod4Mask | ShiftMask, workspace_swap, {.i = 8}},
    {XK_9, Mod4Mask | ShiftMask, workspace_swap, {.i = 9}},

    /* Window manager manipulation */
    {XK_j, Mod4Mask | Mod1Mask, mon_roll_l},
    {XK_k, Mod4Mask | Mod1Mask, mon_roll_r},
    {XK_1, Mod4Mask | Mod1Mask, mon_swap, {.i = 1}},
    {XK_2, Mod4Mask | Mod1Mask, mon_swap, {.i = 2}},
    {XK_3, Mod4Mask | Mod1Mask, mon_swap, {.i = 3}},
    {XK_4, Mod4Mask | Mod1Mask, mon_swap, {.i = 4}},
    {XK_5, Mod4Mask | Mod1Mask, mon_swap, {.i = 5}},
    {XK_6, Mod4Mask | Mod1Mask, mon_swap, {.i = 6}},
    {XK_7, Mod4Mask | Mod1Mask, mon_swap, {.i = 7}},
    {XK_8, Mod4Mask | Mod1Mask, mon_swap, {.i = 8}},
    {XK_9, Mod4Mask | Mod1Mask, mon_swap, {.i = 9}},
    {XK_r, Mod4Mask | Mod1Mask, wm_restart},
    {XK_l, Mod4Mask | Mod1Mask, wm_logout},
    {XK_s,
     Mod4Mask | Mod1Mask,
     window_push,
     {.s = (char *[]){"systemctl", "suspend", NULL}}},

    /* Launchers */
    {XK_space,
     Mod4Mask,
     window_push,
     {.s = (char *[]){"gnome-software", NULL}}},
    {XK_s,
     Mod4Mask,
     window_push,
     {.s = (char *[]){"gnome-control-center", "sound", NULL}}},
    {XK_c, Mod4Mask, window_push, {.s = (char *[]){"google-chrome", NULL}}},
    {XK_f, Mod4Mask, window_push, {.s = (char *[]){"nautilus", NULL}}},
    {XK_z, Mod4Mask, window_push, {.s = (char *[]){"zoom", NULL}}},
    {XK_t, Mod4Mask, window_push, {.s = (char *[]){"kitty", NULL}}},
    {XK_e, Mod4Mask, window_push, {.s = (char *[]){"kitty", "nvim", NULL}}},

    /* Misc */
    {XF86XK_AudioLowerVolume,
     0,
     window_push,
     {.s = (char *[]){"pactl", "set-sink-volume", "@DEFAULT_SINK@", "-10%",
                      NULL}}},
    {XF86XK_AudioRaiseVolume,
     0,
     window_push,
     {.s = (char *[]){"pactl", "set-sink-volume", "@DEFAULT_SINK@", "+10%",
                      NULL}}},
    {XF86XK_AudioMute,
     0,
     window_push,
     {.s = (char *[]){"pactl", "set-sink-mute", "@DEFAULT_SINK@", "toggle"}}},
    {XK_Print, 0, window_push, {.s = (char *[]){"gnome-screenshot", NULL}}},
    {XK_Print,
     Mod4Mask,
     window_push,
     {.s = (char *[]){"gnome-screenshot", "-a", NULL}}}};

/* Graphical customization */
const int gaps = 8;
const float def_ratio = 1.f;
