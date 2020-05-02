#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* -- Global variables and structures -- */
Display *dpy;
Screen *scr;
Window root;
Atom WM_PROTOCOLS, WM_DELETE_WINDOW;
FILE *log_fd;

const int gaps = 8;

typedef struct SWindow SWindow;
struct SWindow {
  Window win;
  SWindow *prev, *next;
};

typedef struct SWorkspace SWorkspace;
struct SWorkspace {
  SWindow *win_stack;
  SWorkspace *prev, *next;
} * wksp_stack;

typedef struct SMonitor SMonitor;
struct SMonitor {
  unsigned int w, h, x, y;
  SMonitor *prev, *next;
} * mon_stack;

/* -- Utility functions -- */
void launch(char **args) {
  pid_t pid = fork();
  if (!pid) {
    execvp(args[0], args);
    fprintf(stderr, "soswm: Unnable to run '%s'\n", args[0]);
    exit(0);
  }
}

Bool send_event(Window win, Atom proto) {
  int n;
  Atom *protos;
  Bool found = False;
  if (XGetWMProtocols(dpy, win, &protos, &n)) {
    while (!found && n--)
      found = protos[n] == proto;
    XFree(protos);
  }
  if (found) {
    XEvent e;
    e.type = ClientMessage;
    e.xclient.window = win;
    e.xclient.message_type = WM_PROTOCOLS;
    e.xclient.format = 32;
    e.xclient.data.l[0] = proto;
    XSendEvent(dpy, win, False, NoEventMask, &e);
  }
  return found;
}

int x_error(Display *dpy, XErrorEvent *err) {
  fprintf(log_fd, "Error\n");
  char error_text[1024];
  XGetErrorText(dpy, err->error_code, error_text, sizeof(error_text));
  fprintf(stderr, "X error: %s\n", error_text);
  return 0;
}

/* -- Window configuration -- */
void update_windows(SWorkspace *wksp) {
  fprintf(log_fd, "UPDATING\n");
  SMonitor *mon = mon_stack;
  if (wksp->win_stack == wksp->win_stack->next) {
    XMoveResizeWindow(dpy, wksp->win_stack->win, mon->x, mon->y, mon->w,
                      mon->h);
  } else {
    Bool v = True;
    unsigned int x = mon->x + gaps / 2, y = mon->y + gaps / 2,
                 w = mon->w - gaps, h = mon->h - gaps;
    SWindow *win;
    for (win = wksp->win_stack; win->next != wksp->win_stack; win = win->next) {
      if (v) {
        XMoveResizeWindow(dpy, win->win, x + gaps / 2, y + gaps / 2,
                          w / 2 - gaps, h - gaps);
        w /= 2;
        x += w;
      } else {
        XMoveResizeWindow(dpy, win->win, x + gaps / 2, y + gaps / 2, w - gaps,
                          h / 2 - gaps);
        h /= 2;
        y += h;
      }
      v = !v;
    }
    XMoveResizeWindow(dpy, win->win, x + gaps / 2, y + gaps / 2, w - gaps,
                      h - gaps);
  }
}

void configure_request(const XConfigureRequestEvent *e) {
  fprintf(log_fd, "CONFIG, %ld\n", e->window);
  /* configure window normally; TODO update this */
  XWindowChanges changes;
  changes.x = e->x;
  changes.y = e->y;
  changes.width = e->width;
  changes.height = e->height;
  changes.border_width = e->border_width;
  changes.sibling = e->above;
  changes.stack_mode = e->detail;
  XConfigureWindow(dpy, e->window, e->value_mask, &changes);
  XSync(dpy, False);
}

void map_request(XMapRequestEvent *e) {
  fprintf(log_fd, "MAP, %ld\n", e->window);
  /* push the new window */
  SWindow *new = malloc(sizeof(SWindow));
  new->win = e->window;
  if (wksp_stack->win_stack) {
    new->prev = wksp_stack->win_stack->prev;
    new->next = wksp_stack->win_stack;
  } else
    new->prev = new->next = new;
  wksp_stack->win_stack = new->prev->next = new->next->prev = new;
  update_windows(wksp_stack);

  /* map and focus updated window */
  XMapWindow(dpy, new->win);
  XSetInputFocus(dpy, new->win, RevertToPointerRoot, CurrentTime);
}

void destroy_notify(XDestroyWindowEvent *e) {
  fprintf(log_fd, "DESTROY\n");
  for (SWorkspace *wksp = wksp_stack; wksp;
       wksp = (wksp->next != wksp_stack) ? wksp->next : NULL) {
    for (SWindow *win = wksp->win_stack; win;
         win = (win->next != wksp->win_stack) ? win->next : NULL) {
      if (win->win == e->window) {
        if (win == win->next)
          wksp->win_stack = NULL;
        else {
          win->next->prev = win->prev;
          win->prev->next = win->next;
          if (wksp->win_stack == win) {
            wksp->win_stack = win->next;
            XSetInputFocus(dpy, win->next->win, RevertToNone, CurrentTime);

            /* TODO make workspace is visible */
            update_windows(wksp);
          }
        }
        free(win);
        return;
      }
    }
  }
}

/* -- Keyboard interaction -- */
void window_quit(XKeyPressedEvent *e) {
  if (e->subwindow != None && !send_event(e->subwindow, WM_DELETE_WINDOW))
    XKillClient(dpy, e->subwindow);
}

void window_roll_l(__attribute__((unused)) XKeyPressedEvent *e) {
  if (wksp_stack->win_stack)
    wksp_stack->win_stack = wksp_stack->win_stack->next;
}

void window_roll_r(__attribute__((unused)) XKeyPressedEvent *e) {
  if (wksp_stack->win_stack)
    wksp_stack->win_stack = wksp_stack->win_stack->prev;
}

void window_swap(XKeyPressedEvent *e) {}

void window_push(XKeyPressedEvent *e) {}

void workspace_new(__attribute__((unused)) XKeyPressedEvent *e) {
  /* if the top workspace is empty, do nothing */
  if (!wksp_stack->win_stack)
    return;
  SWorkspace *wksp = malloc(sizeof(SWorkspace));
  wksp->win_stack = NULL;
  wksp->prev = NULL;
  wksp->next = wksp_stack;
  wksp_stack->prev = wksp;
  wksp_stack = wksp;
}

void workspace_roll_l(__attribute__((unused)) XKeyPressedEvent *e) {}

void workspace_roll_r(__attribute__((unused)) XKeyPressedEvent *e) {}

void workspace_swap(XKeyPressedEvent *e) {}

void wm_restart(__attribute__((unused)) XKeyPressedEvent *e) {}

void wm_logout(__attribute__((unused)) XKeyPressedEvent *e) {
  XCloseDisplay(dpy);
  exit(0);
}

void launcher(__attribute__((unused)) XKeyPressedEvent *e) {
  launch((char *[]){"rofi", "-show", "drun", NULL});
}

void terminal(__attribute__((unused)) XKeyPressedEvent *e) {
  launch((char *[]){"kitty", NULL});
}

void editor(__attribute__((unused)) XKeyPressedEvent *e) {
  launch((char *[]){"kitty", "nvim", NULL});
}

struct {
  unsigned int mask, key, keycode;
  void (*handler)(XKeyPressedEvent *);
} keybinds[] = {
    /* Window management */
    {.mask = Mod4Mask, .key = XK_q, .handler = window_quit},
    {.mask = Mod4Mask, .key = XK_j, .handler = window_roll_l},
    {.mask = Mod4Mask, .key = XK_k, .handler = window_roll_r},
    {.mask = Mod4Mask, .key = XK_1, .handler = window_swap},
    {.mask = Mod4Mask, .key = XK_2, .handler = window_swap},
    {.mask = Mod4Mask, .key = XK_3, .handler = window_swap},
    {.mask = Mod4Mask, .key = XK_4, .handler = window_swap},
    {.mask = Mod4Mask, .key = XK_5, .handler = window_swap},
    {.mask = Mod4Mask, .key = XK_6, .handler = window_swap},
    {.mask = Mod4Mask, .key = XK_7, .handler = window_swap},
    {.mask = Mod4Mask, .key = XK_8, .handler = window_swap},
    {.mask = Mod4Mask, .key = XK_9, .handler = window_swap},
    {.mask = Mod4Mask | ShiftMask, .key = XK_1, .handler = window_push},
    {.mask = Mod4Mask | ShiftMask, .key = XK_2, .handler = window_push},
    {.mask = Mod4Mask | ShiftMask, .key = XK_3, .handler = window_push},
    {.mask = Mod4Mask | ShiftMask, .key = XK_4, .handler = window_push},
    {.mask = Mod4Mask | ShiftMask, .key = XK_5, .handler = window_push},
    {.mask = Mod4Mask | ShiftMask, .key = XK_6, .handler = window_push},
    {.mask = Mod4Mask | ShiftMask, .key = XK_7, .handler = window_push},
    {.mask = Mod4Mask | ShiftMask, .key = XK_8, .handler = window_push},
    {.mask = Mod4Mask | ShiftMask, .key = XK_9, .handler = window_push},

    /* Workspace management */
    {.mask = Mod4Mask | ControlMask, .key = XK_n, .handler = workspace_new},
    {.mask = Mod4Mask | ControlMask, .key = XK_j, .handler = workspace_roll_l},
    {.mask = Mod4Mask | ControlMask, .key = XK_k, .handler = workspace_roll_r},
    {.mask = Mod4Mask | ControlMask, .key = XK_1, .handler = workspace_swap},
    {.mask = Mod4Mask | ControlMask, .key = XK_2, .handler = workspace_swap},
    {.mask = Mod4Mask | ControlMask, .key = XK_3, .handler = workspace_swap},
    {.mask = Mod4Mask | ControlMask, .key = XK_4, .handler = workspace_swap},
    {.mask = Mod4Mask | ControlMask, .key = XK_5, .handler = workspace_swap},
    {.mask = Mod4Mask | ControlMask, .key = XK_6, .handler = workspace_swap},
    {.mask = Mod4Mask | ControlMask, .key = XK_7, .handler = workspace_swap},
    {.mask = Mod4Mask | ControlMask, .key = XK_8, .handler = workspace_swap},
    {.mask = Mod4Mask | ControlMask, .key = XK_9, .handler = workspace_swap},

    /* WM management */
    {.mask = Mod4Mask | Mod1Mask, .key = XK_r, .handler = wm_restart},
    {.mask = Mod4Mask | Mod1Mask, .key = XK_l, .handler = wm_logout},

    /* Launchers */
    {.mask = Mod4Mask, .key = XK_space, .handler = launcher},
    {.mask = Mod4Mask, .key = XK_t, .handler = terminal},
    {.mask = Mod4Mask, .key = XK_e, .handler = editor},
};
const unsigned int NUM_KEYBINDS = sizeof(keybinds) / sizeof(*keybinds);

void key_press(XKeyPressedEvent *e) {
  for (unsigned int i = 0; i < NUM_KEYBINDS; i++) {
    if ((keybinds[i].mask & e->state) && (keybinds[i].keycode == e->keycode))
      keybinds[i].handler(e);
  }
}

/* -- Mouse interaction -- */
void button_press(__attribute__((unused)) XButtonPressedEvent *e) {}

void motion_notify(__attribute__((unused)) XMotionEvent *e) {}

/* -- Main loop -- */
int main() {
  log_fd = fopen("/tmp/soswm.log", "w");
  /* initialize display */
  if (!(dpy = XOpenDisplay(0x0))) {
    fprintf(stderr, "soswm: Cannot open display\n");
    exit(1);
  }
  root = DefaultRootWindow(dpy);
  XSetErrorHandler(x_error);
  XSelectInput(dpy, root, SubstructureNotifyMask | SubstructureRedirectMask);

  /* get screen */
  scr = XDefaultScreenOfDisplay(dpy);

  /* gather monitors; TODO actually determine this */
  mon_stack = malloc(sizeof(SMonitor));
  mon_stack->prev = NULL;
  mon_stack->next = malloc(sizeof(SMonitor));
  mon_stack->next->prev = mon_stack;
  mon_stack->next->next = NULL;
  mon_stack->w = mon_stack->next->w = 1920;
  mon_stack->h = mon_stack->next->h = 1080;
  mon_stack->x = 0;
  mon_stack->y = mon_stack->next->x = mon_stack->next->y = 0;

  /* create initial workspace */
  wksp_stack = malloc(sizeof(SWorkspace));
  wksp_stack->win_stack = NULL;
  wksp_stack->prev = NULL;
  wksp_stack->next = NULL;

  /* initialize communication protocols */
  WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
  WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

  /* bind keys */
  for (unsigned int i = 0; i < NUM_KEYBINDS; i++) {
    keybinds[i].keycode = XKeysymToKeycode(dpy, keybinds[i].key);
    XGrabKey(dpy, keybinds[i].keycode, keybinds[i].mask, root, True,
             GrabModeAsync, GrabModeAsync);
  }

  /* bind buttons */
  /* XGrabButton(dpy, 1, NoEventMask, root, True, */
  /* ButtonPressMask | ButtonReleaseMask | PointerMotionMask, */
  /* GrabModeAsync, GrabModeAsync, None, None); */

  /* run startup programs */
  launch((char *[]){"feh", "--bg-scale",
                    "/home/benwilliamgraham/Pictures/mountain.jpg", NULL});

  /* main loop */
  XSync(dpy, False);
  for (;;) {
    XEvent e[1];
    XNextEvent(dpy, e);

    switch (e->type) {
    case ConfigureRequest:
      configure_request(&e->xconfigurerequest);
      break;
    case MapRequest:
      map_request(&e->xmaprequest);
      break;
    case DestroyNotify:
      destroy_notify(&e->xdestroywindow);
      break;
    case KeyPress:
      key_press(&e->xkey);
      break;
    case ButtonPress:
      button_press(&e->xbutton);
      break;
    case MotionNotify:
      while (XCheckTypedWindowEvent(dpy, e->xmotion.window, MotionNotify, e))
        ;
      motion_notify(&e->xmotion);
      break;
    }
  }
}
