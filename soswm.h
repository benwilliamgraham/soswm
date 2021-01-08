#ifndef SOSWM_H
#define SOSWM_H

/* sosc push workspace */
void push_workspace();

/* sosc pop <window | workspace> */
void pop_window();
void pop_workspace();

/* sosc swap <window | workspace | monitor> <n> */
void swap_window(unsigned int n);
void swap_workspace(unsigned int n);
void swap_monitor(unsigned int n);

/* sosc roll <window | workspace | monitor> <left | right> */
typedef enum { ROLL_LEFT, ROLL_RIGHT } RollDirection;
void roll_window(RollDirection dir);
void roll_workspace(RollDirection dir);
void roll_monitor(RollDirection dir);

/* sosc move window <n> */
void move_window(unsigned int n);

/* sosc replace */
void replace_wm();

/* sosc logout */
void logout_wm();

/* sosc layout workspace <fullscreen | halved | toggle> */
typedef enum { LAYOUT_FULLSCREEN, LAYOUT_HALVED, LAYOUT_TOGGLE } LayoutMode;
void layout_workspace(LayoutMode mode);

/* sosc scale workspace <bigger | smaller | reset> */
typedef enum { SCALE_BIGGER, SCALE_SMALLER, SCALE_RESET } ScaleMode;
void scale_workspace(ScaleMode mode);

/* sosc gap <top | bottom | left | right | inner> <n> */
void gap_top(unsigned int n);
void gap_bottom(unsigned int n);
void gap_left(unsigned int n);
void gap_right(unsigned int n);
void gap_inner(unsigned int n);

#endif /* !SOSWM_H */
