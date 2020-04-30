#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* -- Global variables and structures -- */
Display *dpy;
Window root;
Atom WM_PROTOCOLS, WM_DELETE_WINDOW;

XButtonEvent start;
XWindowAttributes attr;

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

typedef struct SScreen SScreen;
struct SScreen {
  Screen *scr;
  SScreen *prev, *next;
} * scr_stack;

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
  char error_text[1024];
  XGetErrorText(dpy, err->error_code, error_text, sizeof(error_text));
  fprintf(stderr, "X error: %s\n", error_text);
  return 0;
}

/* -- Window configuration -- */
void configure_request(const XConfigureRequestEvent *e) {
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

void map_request(XMapRequestEvent *e) { XMapWindow(dpy, e->window); }

/* -- Keyboard interaction -- */
void new_wksp(__attribute__((unused)) XKeyPressedEvent *e) {
  /* if the top workspace is empty, do nothing */
  if (wksp_stack->win_stack == NULL)
    return;
  SWorkspace *wksp = malloc(sizeof(SWorkspace));
  wksp->win_stack = NULL;
  wksp->prev = NULL;
  wksp->next = wksp_stack;
  wksp_stack->prev = wksp;
  wksp_stack = wksp;
}

void quit(XKeyPressedEvent *e) {
  if (!send_event(e->subwindow, WM_DELETE_WINDOW))
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
    {.mask = Mod4Mask, .keysym = XK_n, .handler = new_wksp},
    {.mask = Mod4Mask | ShiftMask, .keysym = XK_l, .handler = logout},
    {.mask = Mod4Mask | ShiftMask, .keysym = XK_q, .handler = quit},
    {.mask = Mod4Mask, .keysym = XK_space, .handler = launcher},
    {.mask = Mod4Mask, .keysym = XK_t, .handler = terminal},
    {.mask = Mod4Mask, .keysym = XK_e, .handler = editor}};
const size_t NUM_KEYBINDS = sizeof(keybinds) / sizeof(*keybinds);

void key_press(XKeyPressedEvent *e) {
  for (size_t i = 0; i < NUM_KEYBINDS; i++) {
    if ((keybinds[i].mask & e->state) && (keybinds[i].keycode == e->keycode))
      keybinds[i].handler(e);
  }
}

/* -- Mouse interaction -- */
void button_press(XButtonPressedEvent *e) {
  XSetInputFocus(dpy, e->subwindow, RevertToNone, CurrentTime);
  XRaiseWindow(dpy, e->subwindow);
  XGetWindowAttributes(dpy, e->subwindow, &attr);
  start = *e;
}

void motion_notify(XMotionEvent *e) {
  XMoveWindow(dpy, e->subwindow, attr.x + e->x_root - start.x_root,
              attr.y + e->y_root - start.y_root);
}

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

  /* gather screens */
  int num_screens = XScreenCount(dpy);
  fprintf(stderr, "%d screens\n", num_screens);
  SScreen *scr = scr_stack, *prev = NULL;
  for (int i = 0; i < num_screens; i++) {
    scr = malloc(sizeof(SScreen));
    scr->scr = XScreenOfDisplay(dpy, i);
    scr->prev = prev;
    scr->next = NULL;
    if (prev != NULL)
      prev->next = scr;
    prev = scr;
  }

  /* create initial workspace */
  wksp_stack = malloc(sizeof(SWorkspace));
  wksp_stack->win_stack = NULL;
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
  XGrabButton(dpy, 1, Mod4Mask, root, True,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(dpy, 3, Mod4Mask, root, True,
              ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);

  /* run startup programs */
  launch((char *[]){"feh", "--bg-scale",
                    "/home/benwilliamgraham/Pictures/mountain.jpg", NULL});

  /* main loop */
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
