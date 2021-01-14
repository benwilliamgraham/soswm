#ifndef SOSWM_H
#define SOSWM_H

/* sosc push stack */
void push_stack();

/* sosc pop <window | stack> */
void pop_window();
void pop_stack();

/* sosc swap <window | stack> <0...inf> */
void swap_window(unsigned int n);
void swap_stack(unsigned int n);

/* sosc roll <window | stack> <top | bottom> */
typedef enum { ROLL_TOP, ROLL_BOTTOM } RollDirection;
void roll_window(RollDirection dir);
void roll_stack(RollDirection dir);

/* sosc move window <0...inf> */
void move_window(unsigned int n);

/* sosc set gap <0...inf> */
void set_gap(unsigned int n);

/* sosc split screen <WxH+X+Y> ... */
typedef struct {
  unsigned int width, height;
  int x, y;
} Split;
typedef struct {
  Split *splits;
  unsigned int num_splits;
} Splits;
void split_screen(Splits updated_splits);

/* sosc logout */
void logout_wm();

#endif /* !SOSWM_H */
