#include "soswm.h"

#include "server.h"

#include <stdio.h>

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
