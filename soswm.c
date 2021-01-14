#include "soswm.h"

#include "server.h"
#include "stack.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Xlib variables */
Display *dpy;
Window root;
Atom WM_PROTOCOLS, WM_DELETE_WINDOW;

/* X error handler */
int x_error(Display *dpy, XErrorEvent *err) {
  const unsigned int error_msg_size = 1024;
  char error_msg[error_msg_size];
  error_msg[error_msg_size - 1] = '\0';
  XGetErrorText(dpy, err->error_code, error_msg, error_msg_size - 1);
  fprintf(stderr, "soswm: X error: %s\n", error_msg);
  return 0;
}

/* soswm window structure */
typedef struct {
  Bool mapped;
  Window xwin;
} SWindow;

/* Create a new window given an xlib window */
SWindow window_new(Window xwin) {
  return (SWindow){.mapped = False, .xwin = xwin};
}

/* Show a window */
void window_show(SWindow *win) {
  if (!win->mapped) {
    XMapWindow(dpy, win->xwin);
    win->mapped = True;
  }
}

/* Hide a window */
void window_hide(SWindow *win) {
  if (win->mapped) {
    XUnmapWindow(dpy, win->xwin);
    win->mapped = False;
  }
}

/* Kill a window */
void window_kill(SWindow *win) {
  int n;
  Atom *protos;
  // first, try to tell the window to close
  if (XGetWMProtocols(dpy, win->xwin, &protos, &n)) {
    while (n--) {
      if (protos[n] == WM_DELETE_WINDOW) {
        const int LONG_SIZE = 32;
        XEvent e = {.type = ClientMessage};
        e.xclient = (XClientMessageEvent){.window = win->xwin,
                                          .message_type = WM_PROTOCOLS,
                                          .format = LONG_SIZE,
                                          .data = {.l[0] = WM_DELETE_WINDOW}};
        XSendEvent(dpy, win->xwin, False, NoEventMask, &e);
        goto clean_up;
      }
    }
  }
  // if the client has no deletion protocol, forcefully kill it
  XKillClient(dpy, win->xwin);
clean_up:
  XFree(protos);
}

/* Place a window on screen, ensuring it is visible */
void window_place(SWindow *win, int x, int y, unsigned int width,
                  unsigned int height) {
  window_show(win);
  XMoveResizeWindow(dpy, win->xwin, x, y, width, height);
}

/* Focus on a window */
void window_focus(SWindow *win) {
  window_show(win);
  XRaiseWindow(dpy, win->xwin);
  XSetInputFocus(dpy, win->xwin, RevertToParent, CurrentTime);
}

/* Global state */
Stack stacks;
Splits splits;
unsigned int gap;
pthread_t server_thread;

/* Find a window by its X window ID, returning true if found, updating the
 * provided location variables
 */
Bool find_window(Window xwin, SWindow **window, Stack **stack, Split **split) {
  for (unsigned int i = 0; i < stacks.num_items; i++) {
    *stack = stack_at(&stacks, i);
    for (unsigned int j = 0; j < (*stack)->num_items; j++) {
      *window = stack_at(*stack, j);
      if ((*window)->xwin == xwin) {
        *split = i < splits.num_splits ? &splits.splits[i] : NULL;
        return True;
      }
    }
  }
  return False;
}

/* Draw a window stack within a split */
void draw_stack(Stack win_stack, Split split) {
  if (split.width > split.height) {
    unsigned int width = split.width / win_stack.num_items;
    for (unsigned int i = 0; i < win_stack.num_items; i++) {
      window_place(stack_at(&win_stack, i), split.x + width * i, split.y, width,
                   split.height);
    }
  }
}

/* Hide all windows in a window stack */
void hide_stack(Stack win_stack) {
  for (unsigned int i = 0; i < win_stack.num_items; i++) {
    window_hide(stack_at(&win_stack, i));
  }
}

/* Draw all visible windows and hide all non-visible ones */
void draw_all() {
  unsigned int i = 0;
  // draw all visible
  for (; i < splits.num_splits && i < stacks.num_items; i++) {
    draw_stack(*(Stack *)stack_at(&stacks, i), splits.splits[i]);
  }
  // hide all not visible
  for (; i < stacks.num_items; i++) {
    hide_stack(*(Stack *)stack_at(&stacks, i));
  }
  // focus on topmost window, or background
  SWindow *top;
  if (stacks.num_items && (top = stack_at(stack_at(&stacks, 0), 0))) {
    window_focus(top);
  } else {
    XSetInputFocus(dpy, root, RevertToParent, CurrentTime);
  }
}

/* Interface functions */
void push_stack() {
  Stack *win_stack = malloc(sizeof(Stack));
  *win_stack = stack_new();
  stack_push(&stacks, win_stack);
}

void pop_window() {
  Stack *win_stack;
  SWindow *win;
  if ((win_stack = stack_at(&stacks, 0)) && (win = stack_at(win_stack, 0))) {
    window_kill(win);
  }
}

void pop_stack() {
  Stack *win_stack;
  if ((win_stack = stack_pop(&stacks))) {
    free(win_stack);
    draw_all();
  }
}

void roll_window(RollDirection dir) {
  Stack *win_stack;
  if ((win_stack = stack_at(&stacks, 0))) {
    switch (dir) {
    case ROLL_TOP:
      stack_roll_top(win_stack);
      break;
    case ROLL_BOTTOM:
      stack_roll_bottom(win_stack);
      break;
    }
    draw_stack(*win_stack, splits.splits[0]);
  }
}

void roll_stack(RollDirection dir) {
  switch (dir) {
  case ROLL_TOP:
    stack_roll_top(&stacks);
    break;
  case ROLL_BOTTOM:
    stack_roll_bottom(&stacks);
    break;
  }
  draw_all();
}

void move_window(unsigned int n) {
  Stack *from;
  Stack *to;
  SWindow *win;
  if ((from = stack_at(&stacks, 0)) && (to = stack_at(&stacks, n)) &&
      (win = stack_pop(from))) {
    stack_push(to, win);
    draw_all();
  }
}

void swap_window(unsigned int n) {
  Stack *win_stack;
  if ((win_stack = stack_at(&stacks, 0))) {
    stack_swap(win_stack, n);
    draw_stack(*win_stack, splits.splits[0]);
  }
}

void swap_stack(unsigned int n) {
  stack_swap(&stacks, n);
  draw_all();
}

void set_gap(unsigned int n) {
  gap = n;
  draw_all();
}

void split_screen(Splits updated_splits) {
  free(splits.splits);
  splits = updated_splits;
  draw_all();
}

void logout_wm() {
  XCloseDisplay(dpy);
  server_quit();
  exit(0);
}

/* Add a new window */
void add_window(Window xwin) {
  Stack *win_stack;
  if (!(win_stack = stack_at(&stacks, 0))) {
    win_stack = malloc(sizeof(Stack));
    *win_stack = stack_new();
  }
  SWindow *win = malloc(sizeof(SWindow));
  *win = window_new(xwin);
  stack_push(win_stack, win);
  draw_stack(*win_stack, splits.splits[0]);
}

/* Remove a window from wm from the results of a `find_window` */
void remove_window(SWindow *win, Stack *win_stack, Split *split) {
  stack_pop(win_stack);
  if (split) {
    draw_stack(*win_stack, *split);
  }
  free(win);
}

/* Continously accept input from X and server */
void run() {
  for (;;) {
    // check for new X events
    if (XPending(dpy)) {
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
        XMapRequestEvent req = e.xmaprequest;
        XMapWindow(dpy, req.window);
        SWindow *_win;
        Stack *_win_stack;
        Split *_split;
        if (!find_window(req.window, &_win, &_win_stack, &_split)) {
          add_window(req.window);
        }
        break;
      }
      case UnmapNotify: {
        XUnmapEvent ump = e.xunmap;
        SWindow *win;
        Stack *win_stack;
        Split *split;
        if (find_window(ump.window, &win, &win_stack, &split) && win->mapped) {
          remove_window(win, win_stack, split);
        }
        break;
      }
      case DestroyNotify: {
        XDestroyWindowEvent des = e.xdestroywindow;
        SWindow *win;
        Stack *win_stack;
        Split *split;
        if (find_window(des.window, &win, &win_stack, &split)) {
          remove_window(win, win_stack, split);
        }
        break;
      }
      }
    }
    // run pending server command
    server_exec_commmand();
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

  // initialize communication protocols
  WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);
  WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

  // initialize server, run startup program, and start loop
  server_init();
  if (!fork()) {
    const unsigned int startup_path_len = 64;
    char startup_path[startup_path_len];
    snprintf(startup_path, startup_path_len, "%s/.config/soswm/soswmrc",
             getenv("HOME"));
    execl(startup_path, "soswmrc", NULL);
    exit(1);
  }
  run();
}
