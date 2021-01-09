#include "soswm.h"

#include "server.h"
#include "stack.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Xlib variables */
Display *dpy;
Window root;
Atom WM_PROTOCOLS, WM_DELETE_WINDOW;

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
void window_place(SWindow *win, int x, int y, unsigned int width, unsigned int height) {
  window_show(win);
  XMoveResizeWindow(dpy, win->xwin, x, y, width, height);
}

/* Focus on a window */
void window_focus(SWindow *win) {
  window_show(win);
  XRaiseWindow(dpy, win->xwin);
  XSetInputFocus(dpy, win->xwin, RevertToParent, CurrentTime);
}


/* soswm workspace structure */
typedef struct {

} SWorkspace;

/* soswm monitor structure */
typedef struct {

} SMonitor;

/* Runtime stacks */
Stack workspaces, monitors;

void push_workspace() { printf("Workspace pushed\n"); }

void pop_window() { printf("Window popped\n"); }

void pop_workspace() { printf("Workspace popped\n"); }

void swap_window(unsigned int n) { printf("Window swapped %u\n", n); }

void swap_workspace(unsigned int n) { printf("Workspace swapped %u\n", n); }

void swap_monitor(unsigned int n) { printf("Monitor swapped %u\n", n); }

void roll_window(RollDirection dir) {
  printf("Window rolled <left | right>[%u]\n", dir);
}

void roll_workspace(RollDirection dir) {
  printf("Workspace rolled <left | right>[%u]\n", dir);
}

void roll_monitor(RollDirection dir) {
  printf("Monitor rolled <left | right>[%u]\n", dir);
}

void move_window(unsigned int n) { printf("Moved window %u\n", n); }

void replace_wm() { printf("Replaced\n"); }

void logout_wm() { printf("Logged out\n"); }

void layout_workspace(LayoutMode mode) {
  printf("Layout <fullscreen | halved | toggle>[%u]\n", mode);
}

void scale_workspace(ScaleMode mode) {
  printf("Scaled <bigger | smaller | reset>[%u]\n", mode);
}

void gap_top(unsigned int n) { printf("Set top gap to %u\n", n); }

void gap_bottom(unsigned int n) { printf("Set bottom gap to %u\n", n); }

void gap_left(unsigned int n) { printf("Set left gap to %u\n", n); }

void gap_right(unsigned int n) { printf("Set right gap to %u\n", n); }

void gap_inner(unsigned int n) { printf("Set inner gap to %u\n", n); }

int main() {
  server_init();
  for (;;) {
    server_exec_commmands();
  }
}
