#ifndef CONFIG_H
#define CONFIG_H

#include "soswm.h"

#include <X11/XF86keysym.h>
#include <X11/Xutil.h>

/* Cosmetic variables */
extern unsigned int outer_gap;
extern unsigned int inner_gap;
extern float default_win_ratio;

/* Key bindings */
typedef struct KeyBind KeyBind;
struct KeyBind {
  unsigned int key, mask;
  void (*handler)(Arg);
  Arg arg;
  unsigned int keycode;
} extern keybinds[];
extern const unsigned int num_keybinds;

/* Run custom startup procedures */
void startup();

#endif /* !CONFIG_H */
