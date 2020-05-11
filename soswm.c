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

/* -- X-interaction functions -- */
int x_error(Display *dpy, XErrorEvent *err) {
  char error_text[1024];
  XGetErrorText(dpy, err->error_code, error_text, sizeof(error_text));
  fprintf(stderr, "X error: %s\n", error_text);
  return 0;
}

void draw_workspace(SMonitor *mon, SWorkspace *tgt) {
  if (tgt->win_stack) {
    if (tgt->fullscreen || tgt->win_stack == tgt->win_stack->next) {
      XMoveResizeWindow(dpy, tgt->win_stack->win, mon->x, mon->y, mon->w,
                        mon->h);
      /* hide any remaining windows */
      for (SWindow *win = tgt->win_stack->next; win != tgt->win_stack;
           win = win->next)
        XMoveWindow(dpy, win->win, dw, dh);
    } else {
      Bool v = True;
      unsigned int x = mon->x + gaps / 2, y = mon->y + gaps / 2,
                   w = mon->w - gaps, h = mon->h - gaps;
      SWindow *win;
      for (win = tgt->win_stack; win->next != tgt->win_stack; win = win->next) {
        if (v) {
          int lw = tgt->ratio * w / 2;
          XMoveResizeWindow(dpy, win->win, x + gaps / 2, y + gaps / 2,
                            lw - gaps, h - gaps);
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
  }

  /* ensure that the active window is in focus */
  if (wksp_stack->win_stack) {
    XRaiseWindow(dpy, wksp_stack->win_stack->win);
    XSetInputFocus(dpy, wksp_stack->win_stack->win, RevertToParent,
                   CurrentTime);
  } else
    XSetInputFocus(dpy, root, RevertToParent, CurrentTime);
}

void hide_workspace(SWorkspace *tgt) {
  for (SWindow *win = tgt->win_stack; win;
       win = (win->next != tgt->win_stack) ? win->next : NULL)
    XMoveWindow(dpy, win->win, dw, dh);
}

void draw_all(Bool hide_prev, Bool hide_next) {
  /* loop through all monitors */
  SWorkspace *wksp = wksp_stack;
  for (SMonitor *mon = mon_stack; mon && wksp;
       mon = (mon->next != mon_stack) ? mon->next : NULL,
                wksp = (wksp->next != wksp_stack) ? wksp->next : NULL)
    draw_workspace(mon, wksp);

  /* hide the workspace before the first visible */
  if (hide_prev && wksp)
    hide_workspace(wksp_stack->prev);

  /* hide the workspace following the last visible */
  if (hide_next && wksp)
    hide_workspace(wksp);
}

SMonitor *nth_monitor(unsigned int n) {
  for (SMonitor *mon = mon_stack; mon;
       mon = (mon->next != mon_stack) ? mon->next : NULL)
    if (!n--)
      return mon;
  return NULL;
}

SWorkspace *nth_workspace(unsigned int n, SMonitor **mon) {
  *mon = mon_stack;
  for (SWorkspace *wksp = wksp_stack; wksp;
       *mon = (*mon && (*mon)->next != mon_stack) ? (*mon)->next : NULL,
                  wksp = (wksp->next != wksp_stack) ? wksp->next : NULL)
    if (!n--)
      return wksp;
  return NULL;
}

SWindow *nth_window(unsigned int n) {
  for (SWindow *win = wksp_stack->win_stack; win;
       win = (win->next != wksp_stack->win_stack) ? win->next : NULL)
    if (!n--)
      return win;
  return NULL;
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
    e.type = ClientMessage, e.xclient.window = tgt,
    e.xclient.message_type = WM_PROTOCOLS, e.xclient.format = 32,
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
    /* remove window, replacing with NULL if there are none left */
    if (win == win->next)
      wksp->win_stack = NULL;
    else {
      win->next->prev = win->prev, win->prev->next = win->next;
      if (wksp->win_stack == win)
        wksp->win_stack = win->next;

      /* redraw if visible */
      if (mon)
        draw_workspace(mon, wksp);
    }
    free(win);
  }
}

void configure_request(XConfigureRequestEvent *e) {
  /* configure window normally */
  XWindowChanges changes;
  changes.x = e->x, changes.y = e->y, changes.width = e->width,
  changes.height = e->height, changes.border_width = e->border_width,
  changes.sibling = e->above, changes.stack_mode = e->detail,
  XConfigureWindow(dpy, e->window, e->value_mask, &changes);

  /* if the window already exists, redraw it */
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

  /* add the new window to TOS and redraw */
  if (wksp_stack->win_stack)
    new->prev = wksp_stack->win_stack->prev, new->next = wksp_stack->win_stack;
  else
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

  /* clear existing monitors */
  for (SMonitor *mon = mon_stack, *next; mon; mon = next) {
    next = (mon->next == mon_stack) ? mon->next : NULL;
    free(mon);
  }

  /* create new monitors */
  int n_mons;
  XRRMonitorInfo *mons = XRRGetMonitors(dpy, root, False, &n_mons);
  for (XRRMonitorInfo *mon = mons; mon < mons + n_mons; mon++) {
    SMonitor *new = malloc(sizeof(SMonitor));
    new->x = mon->x, new->y = mon->y, new->w = mon->width, new->h = mon->height;
    if (mon_stack)
      new->prev = mon_stack->prev, new->next = mon_stack,
      new->prev->next = new->next->prev = new;
    else
      new->prev = new->next = new;
    mon_stack = new;
  }

  /* create initial workspace if none exists */
  if (!wksp_stack) {
    wksp_stack = malloc(sizeof(SWorkspace));
    wksp_stack->fullscreen = False, wksp_stack->ratio = def_ratio,
    wksp_stack->win_stack = NULL, wksp_stack->prev = wksp_stack,
    wksp_stack->next = wksp_stack;
  }

  /* initialize communication protocols */
  WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
  WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

  /* bind keys */
  for (KeyBind *k = keybinds; k < keybinds + num_keybinds; k++) {
    k->keycode = XKeysymToKeycode(dpy, k->key);
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
    SWindow *win = nth_window(arg.i);
    if (win) {
      /* swap windows and redraw */
      Window tmp = win->win;
      win->win = wksp_stack->win_stack->win;
      wksp_stack->win_stack->win = tmp;
      draw_workspace(mon_stack, wksp_stack);
    }
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
    SMonitor *fmon, *tmon;
    SWorkspace *from, *to;

    /* if no workspace is specified, open in new workspace */
    if (!arg.i) {
      workspace_push((Arg)0u);
      fmon = (mon_stack != mon_stack->next) ? mon_stack->next : NULL,
      from = wksp_stack->next, tmon = mon_stack, to = wksp_stack;
    }

    /* otherwise find specified workspace */
    else
      from = wksp_stack, fmon = mon_stack, to = nth_workspace(arg.i, &tmon);

    if (to) {
      /* remove from old TOS */
      if (tgt == tgt->next)
        from->win_stack = NULL;
      else
        from->win_stack = tgt->prev->next = tgt->next,
        tgt->next->prev = tgt->prev;

      /* add to new TOS */
      if (to->win_stack)
        tgt->prev = to->win_stack->prev, tgt->next = to->win_stack;
      else
        tgt->prev = tgt->next = tgt;
      tgt->prev->next = tgt->next->prev = to->win_stack = tgt;

      /* redraw */
      if (fmon)
        draw_workspace(fmon, from);
      if (tmon)
        draw_workspace(tmon, to);
      else
        hide_workspace(to);
    }
  }
}

void workspace_push(__attribute__((unused)) Arg arg) {
  /* create new workspace */
  SWorkspace *wksp = malloc(sizeof(SWorkspace));
  wksp->win_stack = NULL, wksp->fullscreen = False, wksp->ratio = def_ratio;

  /* push new workspace and redraw all, hiding the bumped workspace */
  wksp->prev = wksp_stack->prev, wksp->next = wksp_stack;
  wksp_stack = wksp->prev->next = wksp->next->prev = wksp;
  draw_all(False, True);
}

void workspace_pop(__attribute__((unused)) Arg arg) {
  SWorkspace *tgt = wksp_stack;
  if (tgt != tgt->next && !tgt->win_stack) {
    /* remove, free, and draw change */
    tgt->prev->next = tgt->next, tgt->next->prev = tgt->prev,
    wksp_stack = tgt->next;
    free(tgt);
    draw_all(False, False);
  }
}

void workspace_swap(Arg arg) {
  SMonitor *mon;
  SWorkspace *tgt = nth_workspace(arg.i, &mon);
  if (tgt) {
    /* save and swap workspaces */
    SWorkspace tmp = *wksp_stack;
    wksp_stack->win_stack = tgt->win_stack,
    wksp_stack->fullscreen = tgt->fullscreen, wksp_stack->ratio = tgt->ratio;
    tgt->win_stack = tmp.win_stack, tgt->fullscreen = tmp.fullscreen,
    tgt->ratio = tmp.ratio;

    /* draw updated workspaces */
    draw_workspace(mon_stack, wksp_stack);
    if (mon)
      draw_workspace(mon, tgt);
    else
      hide_workspace(tgt);
  }
}

void workspace_roll_l(__attribute__((unused)) Arg arg) {
  wksp_stack = wksp_stack->next;
  draw_all(True, False);
}

void workspace_roll_r(__attribute__((unused)) Arg arg) {
  wksp_stack = wksp_stack->prev;
  draw_all(False, True);
}

void workspace_fullscreen(__attribute__((unused)) Arg arg) {
  wksp_stack->fullscreen = !wksp_stack->fullscreen;
  draw_workspace(mon_stack, wksp_stack);
}

void workspace_shrink(__attribute__((unused)) Arg arg) {
  wksp_stack->ratio = max(wksp_stack->ratio - 0.1f, 0.1f);
  draw_workspace(mon_stack, wksp_stack);
}

void workspace_grow(__attribute__((unused)) Arg arg) {
  wksp_stack->ratio = min(wksp_stack->ratio + 0.1f, 1.9f);
  draw_workspace(mon_stack, wksp_stack);
}

void mon_swap(Arg arg) {
  SMonitor *tgt = nth_monitor(arg.i);
  if (tgt) {
    SMonitor tmp = *mon_stack;
    mon_stack->x = tgt->x, mon_stack->y = tgt->y, mon_stack->w = tgt->w,
    mon_stack->h = tgt->h;
    tgt->x = tmp.x, tgt->y = tmp.y, tgt->w = tmp.w, tgt->h = tmp.h;
    draw_all(False, False);
  }
}

void mon_roll_l(__attribute__((unused)) Arg arg) {
  mon_stack = mon_stack->next;
  draw_all(False, False);
}

void mon_roll_r(__attribute__((unused)) Arg arg) {
  mon_stack = mon_stack->prev;
  draw_all(False, False);
}

void wm_restart(__attribute__((unused)) Arg arg) { init(); }

void wm_logout(__attribute__((unused)) Arg arg) { quit(); }

/* -- Main loop -- */
int main() {
  init();
  run();
}
