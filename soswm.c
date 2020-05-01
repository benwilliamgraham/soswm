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

typedef struct SWindow SWindow;
struct SWindow {
  Window win;
  SWindow *prev, *next;
};

typedef struct SWorkspace SWorkspace;
struct SWorkspace {
  SWindow *win_stack;
  size_t num_wins;
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
  printf("ERROR\n");
  char error_text[1024];
  XGetErrorText(dpy, err->error_code, error_text, sizeof(error_text));
  fprintf(stderr, "X error: %s\n", error_text);
  return 0;
}

/* -- Window configuration -- */
Bool find_window(Window win, SWorkspace **wksp_loc, SWindow **win_loc) {
  printf("FINDING\n");
  for (*wksp_loc = wksp_stack; *wksp_loc; *wksp_loc = (*wksp_loc)->next) {
    for (*win_loc = (*wksp_loc)->win_stack; *win_loc;
         *win_loc = (*win_loc)->next) {
      if ((*win_loc)->win == win)
        return True;
    }
  }
  return False;
}

void update_windows(SWorkspace *wksp) {
  printf("UPDATING, %ld\n", wksp->num_wins);
  SMonitor *mon = mon_stack;
  if (wksp->num_wins == 1) {
    XSetInputFocus(dpy, wksp->win_stack->win, RevertToNone, CurrentTime);
    XMoveResizeWindow(dpy, wksp->win_stack->win, mon->x, mon->y, mon->w,
                      mon->h);
  } else if (wksp->num_wins) {
    XSetInputFocus(dpy, wksp->win_stack->win, RevertToNone, CurrentTime);
    XMoveResizeWindow(dpy, wksp->win_stack->win, mon->x, mon->y, mon->w / 2,
                      mon->h);
    size_t i = 0;
    for (SWindow *win = wksp->win_stack->next; win; win = win->next, i++) {
      XMoveResizeWindow(dpy, win->win, mon->x + mon->w / 2,
                        mon->h * i / (wksp->num_wins - 1), mon->w / 2,
                        mon->h / (wksp->num_wins - 1));
    }
  }
}

void configure_request(const XConfigureRequestEvent *e) {
  printf("CONFIG\n");
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
}

void map_request(XMapRequestEvent *e) {
  printf("MAP\n");
  /* push the new window */
  SWindow *new = malloc(sizeof(SWindow));
  new->win = e->window;
  new->prev = NULL;
  new->next = wksp_stack->win_stack;
  if (wksp_stack->win_stack)
    wksp_stack->win_stack->prev = new;
  wksp_stack->win_stack = new;
  wksp_stack->num_wins++;
  update_windows(wksp_stack);

  /* draw updated window */
  XMapWindow(dpy, e->window);
}

void destroy_notify(XDestroyWindowEvent *e) {
  printf("DESTROY\n");
  SWorkspace *wksp = NULL;
  SWindow *win = NULL;
  if (!find_window(e->window, &wksp, &win))
    return;
  if (win->prev)
    win->prev->next = win->next;
  if (win->next)
    win->next->prev = win->prev;
  if (wksp->win_stack == win)
    wksp->win_stack = win->next;
  free(win);
  wksp->num_wins--;

  /* TODO ensure window is visible before redrawing */
  update_windows(wksp);
}

/* -- Keyboard interaction -- */
void new_wksp(__attribute__((unused)) XKeyPressedEvent *e) {
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

void quit(XKeyPressedEvent *e) {
  if (e->subwindow != None && !send_event(e->subwindow, WM_DELETE_WINDOW))
    XKillClient(dpy, e->subwindow);
}

void logout(__attribute__((unused)) XKeyPressedEvent *e) {
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
  unsigned int mask, keysym, keycode;
  void (*handler)(XKeyPressedEvent *);
} keybinds[] = {
    {.mask = Mod1Mask, .keysym = XK_n, .handler = new_wksp},
    {.mask = Mod1Mask | ShiftMask, .keysym = XK_l, .handler = logout},
    {.mask = Mod1Mask | ShiftMask, .keysym = XK_q, .handler = quit},
    {.mask = Mod1Mask, .keysym = XK_space, .handler = launcher},
    {.mask = ShiftMask, .keysym = XK_t, .handler = terminal},
    {.mask = Mod1Mask, .keysym = XK_e, .handler = editor}};
const size_t NUM_KEYBINDS = sizeof(keybinds) / sizeof(*keybinds);

void key_press(XKeyPressedEvent *e) {
  for (size_t i = 0; i < NUM_KEYBINDS; i++) {
    if ((keybinds[i].mask & e->state) && (keybinds[i].keycode == e->keycode))
      keybinds[i].handler(e);
  }
}

/* -- Mouse interaction -- */
void button_press(__attribute__((unused)) XButtonPressedEvent *e) {}

void motion_notify(__attribute__((unused)) XMotionEvent *e) {}

/* -- Main loop -- */
int main() {
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
  wksp_stack->num_wins = 0;
  wksp_stack->prev = NULL;
  wksp_stack->next = NULL;

  /* initialize communication protocols */
  WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
  WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

  /* bind keys */
  for (size_t a = 0; a < NUM_KEYBINDS; a++) {
    keybinds[a].keycode = XKeysymToKeycode(dpy, keybinds[a].keysym);
    XGrabKey(dpy, keybinds[a].keycode, keybinds[a].mask, root, True,
             GrabModeAsync, GrabModeAsync);
  }

  /* bind buttons */
  XGrabButton(dpy, 1, NoEventMask, root, True,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);

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
