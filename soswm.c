#include "config.c"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* -- Global variables and structures -- */
Display *dpy = NULL;
Screen *scr = NULL;
Window root = None;
Atom WM_PROTOCOLS = None, WM_DELETE_WINDOW = None;

typedef struct KeyBind KeyBind;
const unsigned int num_keybinds = sizeof(keybinds) / sizeof(*keybinds),
                   num_programs = sizeof(programs) / sizeof(*programs);

typedef struct SWindow SWindow;
struct SWindow {
  Window win;
  SWindow *prev, *next;
};

typedef struct SWorkspace SWorkspace;
struct SWorkspace {
  Bool fullscreen;
  float ratio;
  SWindow *win_stack;
  SWorkspace *prev, *next;
} *wksp_stack = NULL;

typedef struct SMonitor SMonitor;
struct SMonitor {
  unsigned int w, h, x, y;
  SMonitor *prev, *next;
} *mon_stack = NULL;

/* -- X-interaction functions -- */
int x_error(Display *dpy, XErrorEvent *err) {
  char error_text[1024];
  XGetErrorText(dpy, err->error_code, error_text, sizeof(error_text));
  fprintf(stderr, "X error: %s\n", error_text);
  return 0;
}

void update_windows(SWorkspace *wksp) {
  if (!wksp->win_stack)
    return;
  SMonitor *mon = mon_stack;
  if (wksp->fullscreen || wksp->win_stack == wksp->win_stack->next) {
    XMoveResizeWindow(dpy, wksp->win_stack->win, mon->x, mon->y, mon->w,
                      mon->h);
  } else {
    Bool v = True;
    unsigned int x = mon->x + gaps / 2, y = mon->y + gaps / 2,
                 w = mon->w - gaps, h = mon->h - gaps;
    SWindow *win;
    for (win = wksp->win_stack; win->next != wksp->win_stack; win = win->next) {
      if (v) {
        int lw = wksp->ratio * w / 2;
        XMoveResizeWindow(dpy, win->win, x + gaps / 2, y + gaps / 2, lw - gaps,
                          h - gaps);
        w -= lw;
        x += lw;
      } else {
        int lh = wksp->ratio * h / 2;
        XMoveResizeWindow(dpy, win->win, x + gaps / 2, y + gaps / 2, w - gaps,
                          lh - gaps);
        h -= lh;
        y += lh;
      }
      v = !v;
    }
    XMoveResizeWindow(dpy, win->win, x + gaps / 2, y + gaps / 2, w - gaps,
                      h - gaps);
  }
  XRaiseWindow(dpy, wksp_stack->win_stack->win);
  XSetInputFocus(dpy, wksp_stack->win_stack->win, RevertToParent, CurrentTime);
}

void launch_window(char **cmd) {
  pid_t pid = fork();
  if (!pid) {
    execvp(cmd[0], cmd);
    fprintf(stderr, "soswm: Unnable to run '%s'\n", cmd[0]);
    exit(0);
  }
}

void kill_window(Window win) {
  int n;
  Atom *protos;
  Bool found = False;
  if (XGetWMProtocols(dpy, win, &protos, &n)) {
    while (!found && n--)
      found = protos[n] == WM_DELETE_WINDOW;
    XFree(protos);
  }
  if (found) {
    XEvent e;
    e.type = ClientMessage;
    e.xclient.window = win;
    e.xclient.message_type = WM_PROTOCOLS;
    e.xclient.format = 32;
    e.xclient.data.l[0] = WM_DELETE_WINDOW;
    XSendEvent(dpy, win, False, NoEventMask, &e);
  } else
    XKillClient(dpy, win);
}

void remove_window(Window target) {
  for (SWorkspace *wksp = wksp_stack; wksp;
       wksp = (wksp->next != wksp_stack) ? wksp->next : NULL) {
    for (SWindow *win = wksp->win_stack; win;
         win = (win->next != wksp->win_stack) ? win->next : NULL) {
      if (win->win == target) {
        if (win == win->next) {
          wksp->win_stack = NULL;
        } else {
          win->next->prev = win->prev;
          win->prev->next = win->next;
          if (wksp->win_stack == win)
            wksp->win_stack = win->next;

          /* TODO make sure workspace is visible */
          update_windows(wksp);
        }
        free(win);
        return;
      }
    }
  }
}

void configure_request(XConfigureRequestEvent *e) {
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
  update_windows(wksp_stack);
}

void map_request(XMapRequestEvent *e) {
  XMapWindow(dpy, e->window);
  SWindow *new = malloc(sizeof(SWindow));
  new->win = e->window;
  if (wksp_stack->win_stack) {
    new->prev = wksp_stack->win_stack->prev;
    new->next = wksp_stack->win_stack;
  } else
    new->prev = new->next = new;
  wksp_stack->win_stack = new->prev->next = new->next->prev = new;
  update_windows(wksp_stack);
}

void destroy_notify(XDestroyWindowEvent *e) { remove_window(e->window); }

void unmap_notify(XUnmapEvent *e) { remove_window(e->window); }

void key_press(XKeyPressedEvent *e) {
  for (KeyBind *k = keybinds; k < keybinds + num_keybinds; k++) {
    if ((k->mask == e->state) && (k->keycode == e->keycode))
      k->handler(k->arg);
  }
}

void button_press(__attribute__((unused)) XButtonPressedEvent *e) {}

void motion_notify(__attribute__((unused)) XMotionEvent *e) {}

void init() {
  /* initialize display */
  if (!dpy)
    if (!(dpy = XOpenDisplay(0x0))) {
      fprintf(stderr, "soswm: Cannot open display\n");
      exit(1);
    }
  root = DefaultRootWindow(dpy);
  XSetErrorHandler(x_error);
  XSelectInput(dpy, root, SubstructureNotifyMask | SubstructureRedirectMask);

  /* get screen */
  scr = XDefaultScreenOfDisplay(dpy);

  /* clear then gather monitors; TODO actually determine this */
  for (SMonitor *mon = mon_stack, *next; mon; mon = next) {
    next = (mon->next == mon_stack) ? mon->next : NULL;
    free(mon);
  }
  mon_stack = malloc(sizeof(SMonitor));
  mon_stack->prev = mon_stack;
  mon_stack->next = mon_stack;
  mon_stack->w = 1920;
  mon_stack->h = 1080;
  mon_stack->x = 1920;
  mon_stack->y = 0;

  /* create initial workspace if none exists */
  if (!wksp_stack) {
    wksp_stack = malloc(sizeof(SWorkspace));
    wksp_stack->fullscreen = False;
    wksp_stack->ratio = def_ratio;
    wksp_stack->win_stack = NULL;
    wksp_stack->prev = wksp_stack;
    wksp_stack->next = wksp_stack;
  }

  /* initialize communication protocols */
  WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
  WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

  /* bind keys */
  for (KeyBind *k = keybinds; k < keybinds + num_keybinds; k++) {
    k->keycode = XKeysymToKeycode(dpy, XStringToKeysym(k->key));
    XGrabKey(dpy, k->keycode, k->mask, root, True, GrabModeAsync,
             GrabModeAsync);
  }

  /* bind buttons */
  /* XGrabButton(dpy, 1, NoEventMask, root, True, */
  /* ButtonPressMask | ButtonReleaseMask | PointerMotionMask, */
  /* GrabModeAsync, GrabModeAsync, None, None); */

  /* launch startup programs */
  for (char ***p = programs; p < programs + num_programs; p++)
    launch_window(*p);
}

void run() {
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
    case UnmapNotify:
      unmap_notify(&e->xunmap);
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

void quit() {
  XCloseDisplay(dpy);
  exit(0);
}

/* -- Implementation-independant functions -- */
void window_push(Arg arg) { launch_window(arg.s); }

void window_pop(__attribute__((unused)) Arg arg) {
  if (wksp_stack->win_stack)
    kill_window(wksp_stack->win_stack->win);
}

void window_swap(Arg arg) {
  if (wksp_stack->win_stack) {
    SWindow *win;
    Window tmp;
    for (win = wksp_stack->win_stack; arg.i; win = win->next, arg.i--)
      ;
    tmp = win->win;
    win->win = wksp_stack->win_stack->win;
    wksp_stack->win_stack->win = tmp;
    update_windows(wksp_stack);
  }
}

void window_roll_l(__attribute__((unused)) Arg arg) {
  if (wksp_stack->win_stack) {
    wksp_stack->win_stack = wksp_stack->win_stack->next;
    update_windows(wksp_stack);
  }
}

void window_roll_r(__attribute__((unused)) Arg arg) {
  if (wksp_stack->win_stack) {
    wksp_stack->win_stack = wksp_stack->win_stack->prev;
    update_windows(wksp_stack);
  }
}

void window_move(Arg arg) {}

void workspace_push(__attribute__((unused)) Arg arg) {}

void workspace_pop(__attribute__((unused)) Arg arg) {}

void workspace_swap(Arg arg) {}

void workspace_roll_l(__attribute__((unused)) Arg arg) {}

void workspace_roll_r(__attribute__((unused)) Arg arg) {}

void workspace_fullscreen(__attribute__((unused)) Arg arg) {
  wksp_stack->fullscreen = !wksp_stack->fullscreen;
  update_windows(wksp_stack);
}

void workspace_shrink(__attribute__((unused)) Arg arg) {
  wksp_stack->ratio = (wksp_stack->ratio <= .1) ? .1 : wksp_stack->ratio - .1;
  update_windows(wksp_stack);
}

void workspace_grow(__attribute__((unused)) Arg arg) {
  wksp_stack->ratio = (wksp_stack->ratio >= 1.9) ? 1.9 : wksp_stack->ratio + .1;
  update_windows(wksp_stack);
}

void wm_restart(__attribute__((unused)) Arg arg) {}

void wm_logout(__attribute__((unused)) Arg arg) { quit(); }

/* -- Main loop -- */
int main() {
  init();
  run();
}
