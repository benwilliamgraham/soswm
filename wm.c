#include "wm.h"

#include "server.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Global structures */
Display *dpy;

Window root;

Atom WM_PROTOCOLS, WM_DELETE_WINDOW;

pthread_mutex_t wm_lock;

typedef struct {
  Window *windows; // ordered windows, stored TOS+n...TOS
  unsigned int num_windows;
} WinStack;
#define window_at(stack, n) (stack).windows[(stack).num_windows - (n)-1]

struct {
  WinStack *win_stacks;
  unsigned int num_win_stacks;
} stack_stack = {
    .win_stacks = NULL, // ordered stacks, stored TOS+n...TOS
    .num_win_stacks = 0,
};
#define win_stack_at(n)                                                        \
  stack_stack.win_stacks[stack_stack.num_win_stacks - (n)-1]

Splits split_stack = {
    .splits = NULL, // ordered splits, stored TOS...TOS+n
    .num_splits = 0,
};
#define split_at(n) split_stack.splits[n]

unsigned int gap = 0;

/* Draw stack on given split */
void draw_stack(WinStack stack, Split split) {
  for (unsigned int w = 0; w < stack.num_windows; w++) {
    Window win = window_at(stack, w);
    XMapWindow(dpy, win);
    if (split.width > split.height) {
      unsigned int width = split.width / stack.num_windows;
      XMoveResizeWindow(dpy, win, split.x + w * width + gap / 2,
                                  split.y + gap / 2,
                                  width - gap,
                                  split.height - gap);
    } else {
      unsigned int height = split.height / stack.num_windows;
      XMoveResizeWindow(dpy, win, split.x + gap / 2,
                                  split.y + w * height + gap / 2, 
                                  split.width - gap,
                                  height - gap);
    }
  }
  // re-focus top window
  WinStack tos = win_stack_at(0);
  if (tos.num_windows) {
    Window win = window_at(tos, 0);
    XRaiseWindow(dpy, win);
    XSetInputFocus(dpy, win, RevertToParent, CurrentTime);
  } else {
    XSetInputFocus(dpy, root, RevertToParent, CurrentTime);
  }
}

/* Hide a stack */
void hide_stack(WinStack stack) {
  for (unsigned int w = 0; w < stack.num_windows; w++) {
    XUnmapWindow(dpy, stack.windows[w]);
  }
}

/* Redraw all windows */
void draw_all() {
  unsigned int s = 0;
  // draw all visible
  for (; s < stack_stack.num_win_stacks && s < split_stack.num_splits; s++) {
    draw_stack(win_stack_at(s), split_at(s));
  }
  // hide all not visible
  for (; s < stack_stack.num_win_stacks; s++) {
    hide_stack(win_stack_at(s));
  }
}

/* Return if a window exists, returning the location (NULL for split if it
 * isn't visible)
 */
Bool find_window(Window win, WinStack **win_stack, Split **split) {
  for (unsigned int s = 0; s < stack_stack.num_win_stacks; s++) {
    *win_stack = &win_stack_at(s);
    *split = s < split_stack.num_splits ? &split_at(s) : NULL;
    for (unsigned int w = 0; w < (*win_stack)->num_windows; w++) {
      if ((*win_stack)->windows[w] == win) {
        return True;
      }
    }
  }
  return False;
}

void remove_window(Window win) {
  for (unsigned int s = 0; s < stack_stack.num_win_stacks; s++) {
    WinStack *win_stack = &win_stack_at(s);
    for (unsigned int w = 0; w < win_stack->num_windows; w++) {
      if (win_stack->windows[w] == win) {
        Window *old = win_stack->windows;
        win_stack->num_windows--;
        win_stack->windows = malloc(win_stack->num_windows * sizeof(Window));
        memcpy(win_stack->windows, old, w * sizeof(Window));
        memcpy(win_stack->windows + w, old + w + 1,
               (win_stack->num_windows - w) * sizeof(Window));
        free(old);
      }
    }
  }
  draw_all();
}

/* X error handler */
int x_error(Display *dpy, XErrorEvent *err) {
  const unsigned int error_msg_size = 1024;
  char error_msg[error_msg_size];
  error_msg[error_msg_size - 1] = '\0';
  XGetErrorText(dpy, err->error_code, error_msg, error_msg_size - 1);
  fprintf(stderr, "soswm: X error: %s\n", error_msg);
  return 0;
}

/* Continuously handle X events */
void x_handler() {
  for (;;) {
    // check for new X events
    pthread_mutex_lock(&wm_lock);
    if (!XPending(dpy)) {
      pthread_mutex_unlock(&wm_lock);
      continue;
    }
    XEvent e;
    XNextEvent(dpy, &e);
    switch (e.type) {
    case ConfigureRequest: {
      XConfigureRequestEvent req = e.xconfigurerequest;
      XWindowChanges changes = {
          .x = req.x,
          .y = req.y,
          .width = req.width,
          .height = req.height,
          .border_width = 0,
          .sibling = req.above,
          .stack_mode = req.detail,
      };
      XConfigureWindow(dpy, req.window, req.value_mask, &changes);
      break;
    }
    case MapRequest: {
      Window win = e.xmaprequest.window;
      XMapWindow(dpy, win);
      // if the window exists, redraw it; otherwise, add it
      WinStack *win_stack;
      Split *split;
      if (find_window(win, &win_stack, &split)) {
        if (split) {
          draw_stack(*win_stack, *split);
        }
      } else {
        if (!stack_stack.num_win_stacks) {
          stack_stack.num_win_stacks = 1;
          stack_stack.win_stacks = malloc(sizeof(WinStack));
          stack_stack.win_stacks[0] = (WinStack){
              .windows = NULL,
              .num_windows = 0,
          };
        }
        win_stack = &win_stack_at(0);
        win_stack->num_windows++;
        win_stack->windows = realloc(win_stack->windows,
                                     win_stack->num_windows * sizeof(Window));
        window_at(*win_stack, 0) = win;
        draw_stack(*win_stack, split_at(0));
      }
      break;
    }
    case UnmapNotify: {
      Window win = e.xunmap.window;
      // delete the window if it should currently be visible
      WinStack *win_stack;
      Split *split;
      if (find_window(win, &win_stack, &split) && split) {
        remove_window(win);
      }
      break;
    }
    case DestroyNotify: {
      remove_window(e.xdestroywindow.window);
      break;
    }
    }
    pthread_mutex_unlock(&wm_lock);
  }
}

int main() {
  // initialize display
  if (!(dpy = XOpenDisplay(0))) {
    fprintf(stderr, "soswm: Could not open display\n");
    exit(1);
  }
  root = XDefaultRootWindow(dpy);
  XSetErrorHandler(x_error);
  XSelectInput(dpy, root, SubstructureNotifyMask | SubstructureRedirectMask);
  XInitThreads();

  // set default split
  Screen *scr = XDefaultScreenOfDisplay(dpy);
  split_stack = (Splits){
      .splits = malloc(sizeof(Split)),
      .num_splits = 1,
  };
  split_at(0) = (Split){
      .width = XWidthOfScreen(scr),
      .height = XHeightOfScreen(scr),
      .x = 0,
      .y = 0,
  };

  // initialize communication protocols
  WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
  WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

  // initialize and run server
  pthread_t server_thread;
  pthread_mutex_init(&wm_lock, NULL);
  server_init();
  pthread_create(&server_thread, NULL, server_host, NULL);

  // run startup program
  if (!fork()) {
    const unsigned int startup_path_len = 64;
    char startup_path[startup_path_len];
    snprintf(startup_path, startup_path_len, "%s/.config/soswm/soswmrc",
             getenv("HOME"));
    execl(startup_path, "soswmrc", NULL);
    exit(1);
  }

  // start X event handler
  x_handler();

  return 0;
}

/* Interface functions */
void push_stack() {
  stack_stack.num_win_stacks++;
  stack_stack.win_stacks = realloc(
      stack_stack.win_stacks, stack_stack.num_win_stacks * sizeof(WinStack));
  win_stack_at(0) = (WinStack){
    .windows = NULL,
    .num_windows = 0,
  };
  draw_all();
}

void pop_window() {
  if (stack_stack.num_win_stacks && win_stack_at(0).num_windows) {
    Window win = window_at(win_stack_at(0), 0);
    int n;
    Atom *protos;
    // first, try to tell the window to close
    if (XGetWMProtocols(dpy, win, &protos, &n)) {
      while (n--) {
        if (protos[n] == WM_DELETE_WINDOW) {
          const int LONG_SIZE = 32;
          XEvent e = {.type = ClientMessage};
          e.xclient = (XClientMessageEvent){.window = win,
                                            .message_type = WM_PROTOCOLS,
                                            .format = LONG_SIZE,
                                            .data = {.l[0] = WM_DELETE_WINDOW}};
          XSendEvent(dpy, win, False, NoEventMask, &e);
          goto clean_up;
        }
      }
    }
    // if the client has no deletion protocol, forcefully kill it
    XKillClient(dpy, win);
  clean_up:
    XFree(protos);
  }
}

void pop_stack() {
  if (stack_stack.num_win_stacks && !win_stack_at(0).num_windows) {
    free(win_stack_at(0).windows);
    stack_stack.num_win_stacks--;
    draw_all();
  }
}

void roll_window(RollDirection dir) {
  if (stack_stack.num_win_stacks) {
    WinStack *tos = &win_stack_at(0);
    if (tos->num_windows > 1) {
      Window top = window_at(*tos, 0);
      Window bottom = window_at(*tos, tos->num_windows - 1);
      Window *old = tos->windows;
      tos->windows = malloc(tos->num_windows * sizeof(Window));
      if (dir == ROLL_TOP) {
        memcpy(tos->windows + 1, old, (tos->num_windows - 1) * sizeof(Window));
        window_at(*tos, tos->num_windows - 1) = top;
      } else {
        memcpy(tos->windows, old + 1, (tos->num_windows - 1) * sizeof(Window));
        window_at(*tos, 0) = bottom;
      }
      free(old);
      draw_all();
    }
  }
}

void roll_stack(RollDirection dir) {
  if (stack_stack.num_win_stacks > 1) {
    WinStack top = win_stack_at(0);
    WinStack bottom = win_stack_at(stack_stack.num_win_stacks - 1);
    WinStack *old = stack_stack.win_stacks;
    stack_stack.win_stacks =
        malloc(stack_stack.num_win_stacks * sizeof(WinStack));
    if (dir == ROLL_TOP) {
      memcpy(stack_stack.win_stacks + 1, old,
             (stack_stack.num_win_stacks - 1) * sizeof(WinStack));
      win_stack_at(stack_stack.num_win_stacks - 1) = top;
    } else {
      memcpy(stack_stack.win_stacks, old + 1,
             (stack_stack.num_win_stacks - 1) * sizeof(WinStack));
      win_stack_at(0) = bottom;
    }
    free(old);
    draw_all();
  }
}

void move_window(unsigned int n) {
  if (stack_stack.num_win_stacks > 1 && win_stack_at(0).num_windows && n &&
      n < stack_stack.num_win_stacks) {
    WinStack *from = &win_stack_at(0);
    WinStack *to = &win_stack_at(n);
    Window win = window_at(*from, 0);
    from->num_windows--;
    to->num_windows++;
    to->windows = realloc(to->windows, to->num_windows * sizeof(Window));
    window_at(*to, 0) = win;
    draw_all();
  }
}

void swap_window(unsigned int n) {
  if (stack_stack.num_win_stacks) {
    WinStack win_stack = win_stack_at(0);
    if (n > 0 && n < win_stack.num_windows) {
      Window tos = window_at(win_stack, 0);
      window_at(win_stack, 0) = window_at(win_stack, n);
      window_at(win_stack, n) = tos;
      draw_stack(win_stack, split_at(0));
    }
  }
}

void swap_stack(unsigned int n) {
  if (n > 0 && n < stack_stack.num_win_stacks) {
    WinStack tos = win_stack_at(0);
    win_stack_at(0) = win_stack_at(n);
    win_stack_at(n) = tos;
    draw_all();
  }
}

void set_gap(unsigned int n) {
  gap = n;
  draw_all();
}

void split_screen(Splits updated_split_stack) {
  free(split_stack.splits);
  split_stack = updated_split_stack;
  draw_all();
}

void logout_wm() {
  XCloseDisplay(dpy);
  server_quit();
  pthread_mutex_destroy(&wm_lock);
  exit(0);
}
