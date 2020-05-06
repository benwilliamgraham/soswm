#include "config.c"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* -- Global variables and structures -- */
Display *dpy = NULL;
unsigned int dw, dh;
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

/* -- Util functions -- */
inline float min(float a, float b) { return (a < b) ? a : b; }

inline float max(float a, float b) { return (a > b) ? a : b; }

inline void swap_ptr(void **a, void **b) {
  void *tmp = *a;
  *a = *b;
  *b = tmp;
}

/* -- X-interaction functions -- */
int x_error(Display *dpy, XErrorEvent *err) {
  char error_text[1024];
  XGetErrorText(dpy, err->error_code, error_text, sizeof(error_text));
  fprintf(stderr, "X error: %s\n", error_text);
  return 0;
}

void draw_workspace(SMonitor *mon, SWorkspace *tgt) {
  if (!tgt->win_stack)
    return;
  if (tgt->fullscreen || tgt->win_stack == tgt->win_stack->next) {
    XMoveResizeWindow(dpy, tgt->win_stack->win, mon->x, mon->y, mon->w, mon->h);
  } else {
    Bool v = True;
    unsigned int x = mon->x + gaps / 2, y = mon->y + gaps / 2,
                 w = mon->w - gaps, h = mon->h - gaps;
    SWindow *win;
    for (win = tgt->win_stack; win->next != tgt->win_stack; win = win->next) {
      if (v) {
        int lw = tgt->ratio * w / 2;
        XMoveResizeWindow(dpy, win->win, x + gaps / 2, y + gaps / 2, lw - gaps,
                          h - gaps);
        w -= lw;
        x += lw;
      } else {
        int lh = tgt->ratio * h / 2;
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

void redraw_all() {
  SWorkspace *wksp = wksp_stack;
  for (SMonitor *mon = mon_stack; mon && wksp;
       mon = (mon->next != mon_stack) ? mon->next : NULL,
                wksp = (wksp->next != wksp_stack) ? wksp->next : NULL)
    draw_workspace(mon, wksp);
}

void hide_workspace(SWorkspace *tgt) {
  for (SWindow *win = tgt->win_stack; win;
       win = (win->next != tgt->win_stack) ? win->next : NULL)
    XMoveWindow(dpy, win->win, dw, dh);
}

void update_workspace(SWorkspace *tgt) {
  SWorkspace *wksp = wksp_stack;
  for (SMonitor *mon = mon_stack; mon;
       mon = (mon->next != mon_stack) ? mon->next : NULL)
    if (wksp == tgt) {
      draw_workspace(mon, tgt);
      return;
    }
  hide_workspace(tgt);
}

void launch_window(char **cmd) {
  pid_t pid = fork();
  if (!pid) {
    execvp(cmd[0], cmd);
    fprintf(stderr, "soswm: Unnable to run '%s'\n", cmd[0]);
    exit(0);
  }
}

void kill_window(Window tgt) {
  int n;
  Atom *protos;
  Bool found = False;
  if (XGetWMProtocols(dpy, tgt, &protos, &n)) {
    while (!found && n--)
      found = protos[n] == WM_DELETE_WINDOW;
    XFree(protos);
  }
  if (found) {
    XEvent e;
    e.type = ClientMessage;
    e.xclient.window = tgt;
    e.xclient.message_type = WM_PROTOCOLS;
    e.xclient.format = 32;
    e.xclient.data.l[0] = WM_DELETE_WINDOW;
    XSendEvent(dpy, tgt, False, NoEventMask, &e);
  } else
    XKillClient(dpy, tgt);
}

Bool find_window(Window tgt, SMonitor **mon, SWorkspace **wksp, SWindow **win) {
  for (*mon = mon_stack, *wksp = wksp_stack; *wksp;
       *mon = (*mon && (*mon)->next != mon_stack) ? (*mon)->next : NULL,
      *wksp = ((*wksp)->next != wksp_stack) ? (*wksp)->next : NULL) {
    for (*win = (*wksp)->win_stack; *win;
         *win = ((*win)->next != (*wksp)->win_stack) ? (*win)->next : NULL)
      if ((*win)->win == tgt)
        return True;
  }
  return False;
}

void remove_window(Window tgt) {
  SMonitor *mon;
  SWorkspace *wksp;
  SWindow *win;
  if (find_window(tgt, &mon, &wksp, &win)) {
    if (win == win->next) {
      wksp->win_stack = NULL;
    } else {
      win->next->prev = win->prev;
      win->prev->next = win->next;
      if (wksp->win_stack == win)
        wksp->win_stack = win->next;

      if (mon)
        draw_workspace(mon, wksp);
    }
    free(win);
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
  SMonitor *mon;
  SWorkspace *wksp;
  SWindow *win;
  if (find_window(e->window, &mon, &wksp, &win) && mon)
    draw_workspace(mon, wksp);
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
  draw_workspace(mon_stack, wksp_stack);
}

void destroy_notify(XDestroyWindowEvent *e) { remove_window(e->window); }

void unmap_notify(XUnmapEvent *e) { remove_window(e->window); }

void key_press(XKeyPressedEvent *e) {
  for (KeyBind *k = keybinds; k < keybinds + num_keybinds; k++) {
    if ((k->mask == e->state) && (k->keycode == e->keycode))
      k->handler(k->arg);
  }
}

void init() {
  /* initialize display */
  if (!dpy)
    if (!(dpy = XOpenDisplay(0x0))) {
      fprintf(stderr, "soswm: Cannot open display\n");
      exit(1);
    }
  Screen *scr = XDefaultScreenOfDisplay(dpy);
  dw = XWidthOfScreen(scr), dh = XHeightOfScreen(scr);
  root = DefaultRootWindow(dpy);
  XSetErrorHandler(x_error);
  XSelectInput(dpy, root, SubstructureNotifyMask | SubstructureRedirectMask);

  /* clear then gather monitors */
  for (SMonitor *mon = mon_stack, *next; mon; mon = next) {
    next = (mon->next == mon_stack) ? mon->next : NULL;
    free(mon);
  }
  int n_mons;
  XRRMonitorInfo *mons = XRRGetMonitors(dpy, root, False, &n_mons);
  for (XRRMonitorInfo *mon = mons; mon < mons + n_mons; mon++) {
    SMonitor *new = malloc(sizeof(SMonitor));
    new->x = mon->x;
    new->y = mon->y;
    new->w = mon->width;
    new->h = mon->height;
    if (mon_stack) {
      new->prev = mon_stack->prev;
      new->next = mon_stack;
      mon_stack->prev->next = mon_stack->next->prev = new;
    } else
      new->prev = new->next = new;
    mon_stack = new;
  }

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
    /* find new window */
    SWindow *win = wksp_stack->win_stack;
    for (; arg.i; win = win->next, arg.i--)
      ;

    /* swap windows and redraw */
    Window tmp = win->win;
    win->win = wksp_stack->win_stack->win;
    wksp_stack->win_stack->win = tmp;
    draw_workspace(mon_stack, wksp_stack);
  }
}

void window_roll_l(__attribute__((unused)) Arg arg) {
  if (wksp_stack->win_stack) {
    wksp_stack->win_stack = wksp_stack->win_stack->next;
    draw_workspace(mon_stack, wksp_stack);
  }
}

void window_roll_r(__attribute__((unused)) Arg arg) {
  if (wksp_stack->win_stack) {
    wksp_stack->win_stack = wksp_stack->win_stack->prev;
    draw_workspace(mon_stack, wksp_stack);
  }
}

void window_move(Arg arg) {
  SWindow *tgt = wksp_stack->win_stack;
  if (tgt) {
    /* find new workspace */
    SWorkspace *wksp = wksp_stack;
    for (; arg.i; wksp = wksp->next, arg.i--)
      ;

    /* remove from old TOS */
    if (tgt == tgt->next)
      wksp_stack->win_stack = NULL;
    else
      wksp_stack->win_stack = tgt->prev->next = tgt->next,
      tgt->next->prev = tgt->prev;

    /* add to new TOS */
    if (wksp->win_stack) {
      tgt->prev = wksp->win_stack->prev;
      tgt->next = wksp->win_stack;
    } else
      tgt->prev = tgt->next = tgt;
    tgt->prev->next = tgt->next->prev = wksp->win_stack = tgt;
  }
}

void workspace_push(__attribute__((unused)) Arg arg) {}

void workspace_pop(__attribute__((unused)) Arg arg) {}

void workspace_swap(Arg arg) {
  SWorkspace *wksp = wksp_stack;
  for (; arg.i; wksp = wksp->next, arg.i--)
    ;
  swap_ptr((void *)&wksp->prev, (void *)&wksp_stack->prev);
  swap_ptr((void *)&wksp->next, (void *)&wksp_stack->next);
  swap_ptr((void *)&wksp, (void *)&wksp_stack);
}

void workspace_roll_l(__attribute__((unused)) Arg arg) {
  wksp_stack = wksp_stack->next;
}

void workspace_roll_r(__attribute__((unused)) Arg arg) {
  wksp_stack = wksp_stack->prev;
}

void workspace_fullscreen(__attribute__((unused)) Arg arg) {
  wksp_stack->fullscreen = !wksp_stack->fullscreen;
}

void workspace_shrink(__attribute__((unused)) Arg arg) {
  wksp_stack->ratio = max(wksp_stack->ratio - 0.1f, 0.1f);
}

void workspace_grow(__attribute__((unused)) Arg arg) {
  wksp_stack->ratio = min(wksp_stack->ratio + 0.1f, 1.9f);
}

void mon_roll_l(__attribute__((unused)) Arg arg) {
  mon_stack = mon_stack->next;
}

void mon_roll_r(__attribute__((unused)) Arg arg) {
  mon_stack = mon_stack->prev;
}

void mon_swap(Arg) {}

void wm_restart(__attribute__((unused)) Arg arg) { init(); }

void wm_logout(__attribute__((unused)) Arg arg) { quit(); }

/* -- Main loop -- */
int main() {
  init();
  run();
}
