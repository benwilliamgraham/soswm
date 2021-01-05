# soswm

## The stack-of-stacks window manager.

soswm is a small and fast dynamic window manager.

## Installation

First, create a `config.c` file. The included `example.config.c` can be used as
a template.

Next, run `[sudo] make clean install` to clean, compile, and install soswm.

## Functionality

As the name suggests, soswm is built around stacks.

Rather than having named workspaces, soswm uses a stack of workspaces, with the
active workspace as the TOS (top-of-stack). Each workspace contains a stack of
windows, with the active window as the TOS. Furthermore, being a dynamic window
manager, soswm chooses the location of all windows and keeps the active
workspace and window in the same location for maximum efficiency. Rather than
moving between windows/workspaces, the active window and workspaces revolve
around you.

Both the window and workspace stack supports several operations to reorder the
stack:

* Push: Add a new item onto TOS
* Pop: Remove an item from TOS 
* Swap(n): Swap the positions of the items at TOS and TOS+n
* Roll left: Take the item at TOS and move it to the bottom of the stack
* Roll right: Take the item at the bottom of the stack and move it to TOS
* Move: Take the TOS item from one stack and put it at the TOS of another

Monitors are also stored on a stack that can be similarly manipulated to change
each one's precedence.

Workspaces have further functionality to handle the way in which windows are
drawn. A fullscreen in which only the active window for the given workspace can
be enabled. Also, for cases where there are more than one window in a workspace,
the windows will be drawn with the active window to the left, and each
successive window taking up a ratio of the remaining screen space. This ratio
can be adjusted by 'shrinking' or 'growing' a workspace, with the default value
existing in the configuration. Finally, the gaps between windows can be changed.

## Configuration

The file `config.c` contains the ability to configure the default programs,
keybinds and graphical settings for soswm. Edit the file and re-install. 

The following functions are made availible for keybinds:

* `window_push`: Push a window with the given command of type `char **`
* `window_pop`: Pop the TOS window
* `window_swap`: Swap window at TOS with TOS+n given n of type `unsigned int`
* `window_roll_l`: Roll the window stack left
* `window_roll_r`: Roll the window stack right
* `window_move`: Move TOS window to workspace TOS+n given n of type `unsigned
int`, or into a new workspace if n is 0
* `workspace_push`: Push a new workspace
* `workspace_pop`: Pop the TOS workspace if it is empty
* `workspace_swap`: Swap workspace at TOS with TOS+n given n of type `unsigned
int`
* `workspace_roll_l`: Roll the workspace stack left
* `workspace_roll_r`: Roll the workspace stack right
* `workspace_fullscreen`: Toggle if a workspace is fullscreen
* `workspace_shrink`: Decrease the ratios between successive windows
* `workspace_grow`: Increase the ratios between successive windows
* `mon_swap`: Swap monitor at TOS with TOS+n given n of type `unsigned int`
* `mon_roll_l`: Roll the monitor stack left
* `mon_roll_r`: Roll the monitor stack right
* `wm_refresh`: Refresh the window manager's display and monitor information (this can be used when the number/orientation of monitors has changed)
* `wm_replace`: Destroy the current instance of the window manager, starting a new one (this can be used to update the configuration without destroying the existing windows)
* `wm_logout`: Exit the window manager

The macros `INT_ARG` and `PROG_ARG` can be used for arguments of type `unsigned int` and `char **` respectively.

The following cosmetic variables are also editable at runtime:
* `gap_pixels`: The number of pixels between two adjacent windows
* `default_win_ratio`: The initial split ratio between two windows in a new workspace

A custom routine to be run upon starting soswm can be modified in the function `startup`.
